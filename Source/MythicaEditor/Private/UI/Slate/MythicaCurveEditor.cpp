#include "MythicaCurveEditor.h"

void SMythicaCurveEditor::Construct(const FArguments& InArgs)
{
    Curve = NewObject<UCurveFloat>(
        GetTransientPackage(),
        UCurveFloat::StaticClass(),
        NAME_None,
        RF_Transactional | RF_Public);
    ensure(Curve);

    Curve->AddToRoot();

    Curve->OnUpdateCurve.AddRaw(this, &SMythicaCurveEditor::OnCurveChanged);

    SCurveEditor::Construct(
        SCurveEditor::FArguments()
        .ViewMinInput(InArgs._ViewMinInput)
        .ViewMaxInput(InArgs._ViewMaxInput)
        .ViewMinOutput(InArgs._ViewMinOutput)
        .ViewMaxOutput(InArgs._ViewMaxOutput)
        .XAxisName(InArgs._XAxisName)
        .YAxisName(InArgs._YAxisName)
        .HideUI(true)//InArgs._HideUI)
        .DrawCurve(true)//InArgs._DrawCurve)
        .TimelineLength(InArgs._TimelineLength)
        .AllowZoomOutput(false)//InArgs._AllowZoomOutput)
        .ShowInputGridNumbers(false)//InArgs._ShowInputGridNumbers)
        .ShowOutputGridNumbers(false)//InArgs._ShowOutputGridNumbers)
        .ShowZoomButtons(false)//InArgs._ShowZoomButtons)
        .ZoomToFitHorizontal(false)//InArgs._ZoomToFitHorizontal)
        .ZoomToFitVertical(false));//InArgs._ZoomToFitVertical));

    // Avoid showing tooltips inside of the curve editor
    EnableToolTipForceField(true);

    SetCurveOwner(Curve);
}

SMythicaCurveEditor::~SMythicaCurveEditor()
{
    if (Curve)
    {
        SetCurveOwner(nullptr);

        Curve->OnUpdateCurve.RemoveAll(this);

        Curve->RemoveFromRoot();
    }
}

FReply SMythicaCurveEditor::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    return FReply::Handled();
}

FReply SMythicaCurveEditor::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
    FReply Reply = SCurveEditor::OnMouseButtonDown(MyGeometry, MouseEvent);

    if (Reply.IsEventHandled())
    {
        return Reply;
    }

    if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && Curve)
    {
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Curve);
        return FReply::Handled();
    }

    return FReply::Unhandled();
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
