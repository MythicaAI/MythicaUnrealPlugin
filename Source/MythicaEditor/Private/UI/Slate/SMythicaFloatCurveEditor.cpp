


#include "UI/Slate/SMythicaFloatCurveEditor.h"

#include "SlateOptMacros.h"

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

    //UCurveEditorSettings* CurveEditorSettings = GetSettings();
    //if (CurveEditorSettings)
    //{
    //    CurveEditorSettings->SetTangentVisibility(ECurveEditorTangentVisibility::NoTangents);
    //}

    // Avoid showing tooltips inside of the curve editor
    EnableToolTipForceField(true);

    SetCurveOwner(Curve);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SMythicaFloatCurveEditor::OnUpdateCurve(UCurveBase*, EPropertyChangeType::Type)
{
}
