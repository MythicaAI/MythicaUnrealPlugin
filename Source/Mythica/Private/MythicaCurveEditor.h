#pragma once

#include "SCurveEditor.h"

class SMythicaCurveEditor : public SCurveEditor
{
public:
	SLATE_BEGIN_ARGS(SMythicaCurveEditor)
		: _ViewMinInput(0.0f)
		, _ViewMaxInput(1.0f)
		, _ViewMinOutput(0.0f)
		, _ViewMaxOutput(1.0f)
		, _TimelineLength(1.0f)
		{
			_Clipping = EWidgetClipping::ClipToBounds;
		}

		SLATE_ATTRIBUTE(float, ViewMinInput)
		SLATE_ATTRIBUTE(float, ViewMaxInput)
		SLATE_ATTRIBUTE(float, ViewMinOutput)
		SLATE_ATTRIBUTE(float, ViewMaxOutput)
		SLATE_ATTRIBUTE(float, TimelineLength)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	~SMythicaCurveEditor();

private:
	void OnCurveChanged(UCurveBase* InCurve, EPropertyChangeType::Type InChangeType);

	UCurveFloat* Curve;
};