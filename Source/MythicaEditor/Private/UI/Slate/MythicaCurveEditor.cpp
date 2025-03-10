#include "MythicaCurveEditor.h"

#include "MythicaEditorPrivatePCH.h"

void SMythicaCurveEditor::Construct(const FArguments& InArgs)
{
	SCurveEditor::Construct(
		SCurveEditor::FArguments()
		.ViewMinInput(InArgs._ViewMinInput)
		.ViewMaxInput(InArgs._ViewMaxInput)
		.ViewMinOutput(InArgs._ViewMinOutput)
		.ViewMaxOutput(InArgs._ViewMaxOutput)
		.TimelineLength(InArgs._TimelineLength)
		.AllowZoomOutput(false)
		.ShowInputGridNumbers(false)
		.ShowOutputGridNumbers(false)
		.ShowZoomButtons(false)
		.ZoomToFitHorizontal(false)
		.ZoomToFitVertical(false));

	Curve = NewObject<UCurveFloat>(
		GetTransientPackage(),
		UCurveFloat::StaticClass(),
		NAME_None,
		RF_Transactional | RF_Public);
	Curve->AddToRoot();
	SetCurveOwner(Curve);

	Curve->OnUpdateCurve.AddRaw(this, &SMythicaCurveEditor::OnCurveChanged);
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

void SMythicaCurveEditor::OnCurveChanged(UCurveBase* InCurve, EPropertyChangeType::Type InChangeType)
{
    UE_LOG(LogTemp, Warning, TEXT("Curve changed"));
}
