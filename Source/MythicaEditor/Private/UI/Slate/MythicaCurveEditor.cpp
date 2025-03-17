#include "MythicaCurveEditor.h"

void SMythicaCurveEditor::Construct(const FArguments& InArgs)
{

    if (!IsValid(InArgs._FloatCurve))
    {
        Curve = NewObject<UCurveFloat>(
            GetTransientPackage(),
            UCurveFloat::StaticClass(),
            NAME_None,
            RF_Transactional | RF_Public);
        ensure(Curve);
        
        Curve->AddToRoot();
    }
    else
    {
        Curve = InArgs._FloatCurve;
    }
    

    //Curve->OnUpdateCurve.AddRaw(this, &SMythicaCurveEditor::OnCurveChanged);

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

    // Avoid showing tooltips inside of the curve editor
    EnableToolTipForceField(true);

    SetCurveOwner(Curve);
}

SMythicaCurveEditor::~SMythicaCurveEditor()
{
    if (Curve)
    {
        SetCurveOwner(nullptr);

        //Curve->OnUpdateCurve.RemoveAll(this);

        Curve->RemoveFromRoot();
    }
}

FReply SMythicaCurveEditor::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    return FReply::Handled();
}

FReply SMythicaCurveEditor::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{

    if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && Curve)
    {
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Curve);
        return FReply::Handled();
    }

    return SCurveEditor::OnMouseButtonDown(MyGeometry, MouseEvent);
}

void SMythicaCurveEditor::OnCurveChanged(UCurveBase* InCurve, EPropertyChangeType::Type InChangeType)
{
    // Create a new UCurveFloat asset in the transient package.
    //Curve = NewObject<UCurveFloat>(
    //    GetTransientPackage(),
    //    UCurveFloat::StaticClass(),
    //    NAME_None,
    //    RF_Transient
    //);
    //
    //if (InCurve)
    //{
    //     Optionally, add some keys to the curve so that it has data.
    //     This example adds keys at time 0 and 1.
    //    Curve->FloatCurve.AddKey(0.f, 0.f);
    //    Curve->FloatCurve.AddKey(1.f, 1.f);
    //
    //    GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(InCurve);
    //}
}
