


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

    SCurveEditor::Construct(
        SCurveEditor::FArguments()
        .ViewMinInput(InArgs._ViewMinInput)
        .ViewMaxInput(InArgs._ViewMaxInput)
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
    return SCurveEditor::OnMouseButtonUp(MyGeometry, MouseEvent);
}

FReply SMythicaFloatCurveEditor::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    return SCurveEditor::OnMouseButtonDown(MyGeometry, MouseEvent);
}

TOptional<int32> SMythicaFloatCurveEditor::GetNumCurveKeys() const
{
    return TOptional<int32>();
}

TOptional<float> SMythicaFloatCurveEditor::GetCurveKeyPosition(const int32 Index) const
{
    return TOptional<float>();
}

TOptional<float> SMythicaFloatCurveEditor::GetCurveKeyValue(const int32 Index) const
{
    return TOptional<float>();
}

TOptional<ERichCurveInterpMode> SMythicaFloatCurveEditor::GetCurveKeyInterpolationType(const int32 Index) const
{
    return TOptional<ERichCurveInterpMode>();
}

void SMythicaFloatCurveEditor::OnUpdateCurve(UCurveBase* Base, EPropertyChangeType::Type Type)
{
    FString TypeString;
    if (Type & EPropertyChangeType::ArrayAdd)
    {
        TypeString = TEXT("Array Add");
    }
    else if (Type & EPropertyChangeType::ArrayRemove)
    {
        TypeString = TEXT("Array Add");
    }
    else if (Type & EPropertyChangeType::ArrayClear)
    {
        TypeString = TEXT("Array Clear");
    }
    else if (Type & EPropertyChangeType::ArrayMove)
    {
        TypeString = TEXT("Array Move");
    }
    else if (Type & EPropertyChangeType::Interactive)
    {
        TypeString = TEXT("Interactive");
    }
    else if (Type & EPropertyChangeType::ValueSet)
    {
        TypeString = TEXT("ValueSet");
    }

    UE_LOG(LogMythicaEditor, Warning, TEXT("Curve Update: %s"), *TypeString);
}
