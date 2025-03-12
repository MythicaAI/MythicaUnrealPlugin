

#pragma once

#include "SMythicaCurveEditorBase.h"

#include "CurveEditorSettings.h"
#include "Curves/CurveFloat.h"
#include "SCurveEditor.h"
//#include "Widgets/SCompoundWidget.h"

/**
 * Mythica Float Curve Editor
 *
 * 
 */
class SMythicaFloatCurveEditor : public SMythicaCurveEditorBase<SCurveEditor>
{

public:

    SLATE_BEGIN_ARGS(SMythicaFloatCurveEditor)
        : _ViewMinInput(0.0f)
        , _ViewMaxInput(1.0f)
        , _ViewMinOutput(0.0f)
        , _ViewMaxOutput(1.0f)
        , _InputSnap(0.1f)
        , _OutputSnap(0.05f)
        , _InputSnappingEnabled(false)
        , _OutputSnappingEnabled(false)
        , _ShowTimeInFrames(false)
        , _TimelineLength(1.0f)
        , _DesiredSize(FVector2D::ZeroVector)
        , _DrawCurve(true)
        , _HideUI(true)
        , _AllowZoomOutput(false)
        , _AlwaysDisplayColorCurves(false)
        , _ZoomToFitVertical(false)
        , _ZoomToFitHorizontal(false)
        , _ShowZoomButtons(false)
        , _XAxisName("X")
        , _YAxisName("Y")
        , _ShowInputGridNumbers(false)
        , _ShowOutputGridNumbers(false)
        , _ShowCurveSelector(false)
        , _GridColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.3f))
        {
            _Clipping = EWidgetClipping::ClipToBounds;
        }

        SLATE_ATTRIBUTE(float, ViewMinInput)
        SLATE_ATTRIBUTE(float, ViewMaxInput)
        SLATE_ATTRIBUTE(TOptional<float>, DataMinInput)
        SLATE_ATTRIBUTE(TOptional<float>, DataMaxInput)
        SLATE_ATTRIBUTE(float, ViewMinOutput)
        SLATE_ATTRIBUTE(float, ViewMaxOutput)
        SLATE_ATTRIBUTE(float, InputSnap)
        SLATE_ATTRIBUTE(float, OutputSnap)
        SLATE_ATTRIBUTE(bool, InputSnappingEnabled)
        SLATE_ATTRIBUTE(bool, OutputSnappingEnabled)
        SLATE_ATTRIBUTE(bool, ShowTimeInFrames)
        SLATE_ATTRIBUTE(float, TimelineLength)
        SLATE_ATTRIBUTE(FVector2D, DesiredSize)
        SLATE_ATTRIBUTE(bool, AreCurvesVisible)
        SLATE_ARGUMENT(bool, DrawCurve)
        SLATE_ARGUMENT(bool, HideUI)
        SLATE_ARGUMENT(bool, AllowZoomOutput)
        SLATE_ARGUMENT(bool, AlwaysDisplayColorCurves)
        SLATE_ARGUMENT(bool, ZoomToFitVertical)
        SLATE_ARGUMENT(bool, ZoomToFitHorizontal)
        SLATE_ARGUMENT(bool, ShowZoomButtons)
        SLATE_ARGUMENT(TOptional<FString>, XAxisName)
        SLATE_ARGUMENT(TOptional<FString>, YAxisName)
        SLATE_ARGUMENT(bool, ShowInputGridNumbers)
        SLATE_ARGUMENT(bool, ShowOutputGridNumbers)
        SLATE_ARGUMENT(bool, ShowCurveSelector)
        SLATE_ARGUMENT(FLinearColor, GridColor)
        //SLATE_EVENT(FOnSetInputViewRange, OnSetInputViewRange)
        //SLATE_EVENT(FOnSetOutputViewRange, OnSetOutputViewRange)
        //SLATE_EVENT(FOnSetAreCurvesVisible, OnSetAreCurvesVisible)
        SLATE_EVENT(FSimpleDelegate, OnCreateAsset)

    SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

private:

    void OnUpdateCurve(UCurveBase*, EPropertyChangeType::Type);

private:

    UCurveFloat* Curve;

    FDelegateHandle OnUpdateCurveDelegateHandle;

};
