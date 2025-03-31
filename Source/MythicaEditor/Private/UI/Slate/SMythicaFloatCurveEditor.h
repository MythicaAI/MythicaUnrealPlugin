

#pragma once

#include "SMythicaBaseCurveEditor.h"

#include "Curves/CurveFloat.h"
#include "SCurveEditor.h"

/**
 * Mythica Float Curve Provider
 *
 * This provides an interface into the Mythica data set that allows us direct access to the
 * property handle. This allows for us to manage the data flow from the editor to our backend.
 * 
 * Note: We may want to introduce a new asset type for the Curves themselves though.
 */
class FMythicaFloatCurveProvider : public TMythicaBaseDataProvider<
    float, FMythicaParameterCurve, FMythicaCurvePoint>
{
public:

    explicit FMythicaFloatCurveProvider(TWeakPtr<IPropertyHandle> InHandle, int32_t InParamIndex)
        : TMythicaBaseDataProvider(InHandle, InParamIndex)
    {
    }
};

/**
 * Mythica Float Curve Editor
 *
 * A widget that is designed to utilize the base curve editor in a limited context. This allows us to easily
 * edit any float curve data directly in the details pannel.
 */
class SMythicaFloatCurveEditor : public SMythicaBaseCurveEditor<SCurveEditor, FMythicaFloatCurveProvider>
{

public:

    SLATE_BEGIN_ARGS(SMythicaFloatCurveEditor)
        : _ViewMinInput(-0.01f)
        , _ViewMaxInput(1.01f)
        , _DataMinInput(0.0f)
        , _DataMaxInput(1.0f)
        , _ViewMinOutput(-0.01f)
        , _ViewMaxOutput(1.01f)
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
        // ~Begin Mythica Extensions
        SLATE_ARGUMENT(TSharedPtr<FMythicaFloatCurveProvider>, DataProvider)
        SLATE_EVENT(FOnCurveChanged, OnCurveChanged)
        // ~End Mythica Extensions
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
        SLATE_EVENT(FOnSetInputViewRange, OnSetInputViewRange)
        SLATE_EVENT(FOnSetOutputViewRange, OnSetOutputViewRange)
        SLATE_EVENT(FOnSetAreCurvesVisible, OnSetAreCurvesVisible)
        SLATE_EVENT(FSimpleDelegate, OnCreateAsset)

    SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

    ~SMythicaFloatCurveEditor();

    FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
    FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

protected:

    /**
     * BaseCurveEditor Interface
     */
    virtual TOptional<int32> GetNumCurveKeys() const;
    virtual TOptional<float> GetCurveKeyPosition(const int32 Index) const;
    virtual TOptional<ValueType> GetCurveKeyValue(const int32 Index) const;
    virtual TOptional<ERichCurveInterpMode> GetCurveKeyInterpolationType(
        const int32 Index) const;

    virtual void SyncCurveKeys();

private:

    void OnUpdateCurve(UCurveBase*, EPropertyChangeType::Type);

private:

    /**
     * We only commit the values to the parameter stack after they have been committed.
     * Otherwise, OnUpdateCurve would get called every tick while doing a drag move.
     */
    bool bIsMouseButtonDown = false;

    /**
     * The visual representation of our internal curves so that we dont have to redesign an entire curve editor.
     * This is stored as transient on the widget so do not rely on this to stored persistent state.
     */
    UCurveFloat* Curve;

    /** A managed event that eliminates event spamming while actively interacting with the UI. */
    FDelegateHandle OnUpdateCurveDelegateHandle;

};
