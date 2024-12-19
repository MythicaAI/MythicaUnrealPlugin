#include "MythicaCurveEditor.h"

void SMythicaCurveEditor::Construct(const FArguments& InArgs)
{
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

	Curve = NewObject<UCurveFloat>(
		GetTransientPackage(),
		UCurveFloat::StaticClass(),
		NAME_None,
		RF_Transactional | RF_Public);
	Curve->AddToRoot();
	SetCurveOwner(Curve);
}

SMythicaCurveEditor::~SMythicaCurveEditor()
{
    if (Curve)
    {
		SetCurveOwner(nullptr);
		Curve->RemoveFromRoot();
    }
}