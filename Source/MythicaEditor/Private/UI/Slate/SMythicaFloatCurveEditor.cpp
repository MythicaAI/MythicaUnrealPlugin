


#include "UI/Slate/SMythicaFloatCurveEditor.h"

#include "Editor/CurveEditor/Public/CurveEditorSettings.h"
#include "SlateOptMacros.h"

#include "MythicaEditorPrivatePCH.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SMythicaFloatCurveEditor::Construct(const FArguments& InArgs)
{
    Curve = NewObject<UCurveFloat>(
        GetTransientPackage(),
        UCurveFloat::StaticClass(),
        NAME_None,
        RF_Transactional | RF_Public);
    ensure(Curve);

    Curve->AddToRoot();

    OnUpdateCurveDelegateHandle = Curve->OnUpdateCurve.AddRaw(
        this, &SMythicaFloatCurveEditor::OnUpdateCurve);

    DataProvider = InArgs._DataProvider;
    OnCurveChangedDelegate = InArgs._OnCurveChanged;

    SCurveEditor::Construct(
        SCurveEditor::FArguments()
        .ViewMinInput(InArgs._ViewMinInput)
        .ViewMaxInput(InArgs._ViewMaxInput)
        .DataMinInput(InArgs._DataMinInput)
        .DataMaxInput(InArgs._DataMaxInput)
        .ViewMinOutput(InArgs._ViewMinOutput)
        .ViewMaxOutput(InArgs._ViewMaxOutput)
        .XAxisName(InArgs._XAxisName)
        .YAxisName(InArgs._YAxisName)
        .HideUI(InArgs._HideUI)
        .DrawCurve(InArgs._DrawCurve)
        .TimelineLength(InArgs._TimelineLength)
        .AllowZoomOutput(InArgs._AllowZoomOutput)
        .ShowInputGridNumbers(InArgs._ShowInputGridNumbers)
        .ShowOutputGridNumbers(InArgs._ShowOutputGridNumbers)
        .ShowZoomButtons(InArgs._ShowZoomButtons)
        .ZoomToFitHorizontal(InArgs._ZoomToFitHorizontal)
        .ZoomToFitVertical(InArgs._ZoomToFitVertical));

    UCurveEditorSettings* CurveEditorSettings = GetSettings();
    if (CurveEditorSettings)
    {
        CurveEditorSettings->SetTangentVisibility(ECurveEditorTangentVisibility::NoTangents);
    }

    // Avoid showing tooltips inside of the curve editor
    EnableToolTipForceField(true);

    SetCurveOwner(Curve);

    SyncCurveKeys();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SMythicaFloatCurveEditor::~SMythicaFloatCurveEditor()
{
    if (Curve)
    {
        SetCurveOwner(nullptr);

        Curve->OnUpdateCurve.Remove(OnUpdateCurveDelegateHandle);

        // Remove the ramp curve to root so it can be garbage collected
        Curve->RemoveFromRoot();
    }
}

FReply SMythicaFloatCurveEditor::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    bIsMouseButtonDown = false;

    return SCurveEditor::OnMouseButtonUp(MyGeometry, MouseEvent);
}

FReply SMythicaFloatCurveEditor::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    bIsMouseButtonDown = true;

    return SCurveEditor::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SMythicaFloatCurveEditor::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    return FReply::Handled();
}

TOptional<int32> SMythicaFloatCurveEditor::GetNumCurveKeys() const
{
    if (!Curve)
    {
        return TOptional<int32>();
    }

    return Curve->FloatCurve.GetNumKeys();
}

TOptional<float> SMythicaFloatCurveEditor::GetCurveKeyPosition(const int32 Index) const
{
    if (!Curve || !Curve->FloatCurve.Keys.IsValidIndex(Index))
    {
        return TOptional<float>();
    }

    // Time is treated as the X Component of the Unreal Curve graph. Equivalent to Houdini's Position.
    return Curve->FloatCurve.Keys[Index].Time;
}

TOptional<float> SMythicaFloatCurveEditor::GetCurveKeyValue(const int32 Index) const
{
    if (!Curve || !Curve->FloatCurve.Keys.IsValidIndex(Index))
    {
        return TOptional<float>();
    }

    // Value is treated as the Y Component of the Unreal Curve graph. Equivalent to Houdini's Value.
    return Curve->FloatCurve.Keys[Index].Value;
}

TOptional<ERichCurveInterpMode> SMythicaFloatCurveEditor::GetCurveKeyInterpolationType(const int32 Index) const
{
    if (!Curve || !Curve->FloatCurve.Keys.IsValidIndex(Index))
    {
        return TOptional<ERichCurveInterpMode>();
    }

    return Curve->FloatCurve.Keys[Index].InterpMode.GetValue();
}

void SMythicaFloatCurveEditor::ResetToDefault()
{
    if (!Curve || !DataProvider.IsValid())
    {
        return;
    }

    FRichCurve& FloatCurve = Curve->FloatCurve;

    FloatCurve.Reset();

    TArray<FMythicaCurvePoint> Defaults = DataProvider->GetDefaultPoints();
    for (const FMythicaCurvePoint& Default : Defaults)
    {
        ERichCurveInterpMode RichCurveInterpMode;
        switch (Default.InterpType)
        {
        case EMythicaCurveInterpolationType::MCIT_Linear:
            RichCurveInterpMode = RCIM_Linear;
            break;
        case EMythicaCurveInterpolationType::MCIT_Constant:
            RichCurveInterpMode = RCIM_Constant;
            break;
        case EMythicaCurveInterpolationType::MCIT_Bezier:
        case EMythicaCurveInterpolationType::MCIT_BSpline:
        case EMythicaCurveInterpolationType::MCIT_Catmull_Rom:
        case EMythicaCurveInterpolationType::MCIT_Hermite:
        case EMythicaCurveInterpolationType::MCIT_Monotone_Cubic:
            RichCurveInterpMode = RCIM_Cubic;
            break;
        case EMythicaCurveInterpolationType::MCIT_Invalid:
        default:
            RichCurveInterpMode = RCIM_None;
            break;
        }

        const FKeyHandle KeyHandle = FloatCurve.AddKey(
            Default.Pos,
            Default.GetValueWithType<float>());

        FloatCurve.SetKeyInterpMode(KeyHandle, RichCurveInterpMode);
    }
}

void SMythicaFloatCurveEditor::SyncCurveKeys()
{
    if (!Curve || !DataProvider.IsValid())
    {
        return;
    }

    FRichCurve& FloatCurve = Curve->FloatCurve;

    FloatCurve.Reset();

    const int32 PointCount = DataProvider->GetPointCount();
    for (int32 i = 0; i < PointCount; ++i)
    {
        ERichCurveInterpMode RichCurveInterpMode = DataProvider->GetPointRichCurveInterpolationType(i).GetValue();

        const FKeyHandle KeyHandle = FloatCurve.AddKey(
            DataProvider->GetPointPosition(i).GetValue(),
            DataProvider->GetPointValue(i).GetValue());

        FloatCurve.SetKeyInterpMode(KeyHandle, RichCurveInterpMode);
    }
}

void SMythicaFloatCurveEditor::OnUpdateCurve(UCurveBase* Base, EPropertyChangeType::Type Type)
{
    // To prevent updating data while actively moving points.
    if (bIsMouseButtonDown)
    {
        return;
    }

    OnCurveChanged();
}
