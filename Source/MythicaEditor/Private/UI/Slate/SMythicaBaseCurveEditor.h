

#pragma once

#include "CoreMinimal.h"

#include "MythicaTypes.h"
#include "Curves/RealCurve.h"

#include "MythicaEditorPrivatePCH.h"

#define LOCTEXT_NAMESPACE MYTHICA_LOCTEXT_NAMESPACE

// Inspired by Houdini's CurveEditor design

//class IMythicaCurveEditor
//{
//public:
//
//    /** Used so that all curve types can handle a refresh to its contained data. */
//    virtual void RefreshCurveKeys() = 0;
//};

template<typename InValueType, typename InParamType, typename InPointType>
class TMythicaBaseDataProvider
{

public:

    using ValueType = InValueType;
    using ParamType = InParamType;
    using PointType = InPointType;

public:

    explicit TMythicaBaseDataProvider(TWeakPtr<IPropertyHandle> InHandle)
        : ParameterHandle(InHandle)
    {
    }

    PointType*
        GetRampPoint(const int32 Index) const
    {
        return nullptr;
    }

    /** Gets the number of points the ramp currently has. */
    int32
        GetPointCount() const
    {
        return 0;
    }

    bool
        InsertRampPoint(const int32 Index)
    {
        return true;
    }

    bool
        InsertRampPoint(
            const int32 Index,
            const float Position,
            const ValueType Value,
            const EMythicaCurveInterpolationType InterpolationType)
    {
        return true;
    }

    bool
        DeleteRampPoints(const TArrayView<const int32> Indices)
    {
        return true;
    }

    bool
        DeleteRampPoint(const int32 Index)
    {
        return DeleteRampPoints(MakeArrayView(&Index, 1));
    }

    /** @returns Position if successful, unset optional if unsuccessful. */
    TOptional<float>
        GetRampPointPosition(const int32 Index) const
    {
        if (PointType* const Point = GetRampPoint(Index))
        {
            return Point->Position;
        }

        return {};
    }

    /** @returns Value if successful, unset optional if unsuccessful. */
    TOptional<ValueType>
        GetRampPointValue(const int32 Index) const
    {
        if (PointType* const Point = GetRampPoint(Index))
        {
            return Point->Value;
        }

        return {};
    }

    /** @returns Interpolation if successful, unset optional if unsuccessful. */
    TOptional<EMythicaCurveInterpolationType>
        GetRampPointInterpolationType(const int32 Index) const
    {
        if (PointType* const Point = GetRampPoint(Index))
        {
            return Point->Interpolation;
        }

        return {};
    }

private:

    ParamType* GetParameter() const
    {
        if (!ParameterHandle || !ParameterHandle->IsValidHandle())
        {
            return nullptr;
        }

        //ParameterHandle->
    }

private:

    TWeakPtr<IPropertyHandle> ParameterHandle;

};

DECLARE_DELEGATE(FOnCurveChanged);

/**
 * Mythica Curve Editor Base
 *
 * A template type that combines our parser type (i.e. Houdini Float Ramp Parser, Houdini Color Ramp Parser) with the equivalent unreal slate widget type.
 * 
 * @type BaseClass: The base unreal class.
 * @type CurveParserType: The data type we are converting from.
 */
template<typename BaseClass, typename DataProviderType>
class SMythicaBaseCurveEditor : public BaseClass/*, public IMythicaCurveEditor*/
{

    using ProviderType = DataProviderType;
    using ValueType = typename DataProviderType::ValueType;
    using ParamType = typename DataProviderType::ParamType;
    using PointType = typename DataProviderType::PointType;

protected:

    /**
     * Get data about the curve used by the editor widget.
     *
     * @return Unset optional is only returned if and only if the curve is invalid or if the index
     *         is out of bounds.
     */
    virtual TOptional<int32> GetNumCurveKeys() const = 0;
    virtual TOptional<float> GetCurveKeyPosition(const int32 Index) const = 0;
    virtual TOptional<ValueType> GetCurveKeyValue(const int32 Index) const = 0;
    virtual TOptional<ERichCurveInterpMode> GetCurveKeyInterpolationType(
        const int32 Index) const = 0;

    void OnCurveChanged()
    {
        UE_LOG(LogMythicaEditor, Warning, TEXT("Curve Updated %s"), *DataProvider->ParameterHandle->GeneratePathToProperty());
    }

private:

    /**
     * Since there are fewer Unreal interpolation types than Houdini interpolation types, we map
     * multiple of our Mythica interpolation types to Unreal's cubic interpolation type.
     */
    static bool
        IsInterpolationEquivalent(
            const ERichCurveInterpMode UnrealInterpolation,
            const EMythicaCurveInterpolationType MythicaInterpolation)
    {
        switch (UnrealInterpolation)
        {
        case RCIM_Linear:
            return MythicaInterpolation == EMythicaCurveInterpolationType::MCIT_Linear;
        case RCIM_Constant:
            return MythicaInterpolation == EMythicaCurveInterpolationType::MCIT_Constant;
        case RCIM_Cubic:
            switch (MythicaInterpolation)
            {
            case EMythicaCurveInterpolationType::MCIT_Bezier:
            case EMythicaCurveInterpolationType::MCIT_BSpline:
            case EMythicaCurveInterpolationType::MCIT_Catmull_Rom:
            case EMythicaCurveInterpolationType::MCIT_Hermite:
            case EMythicaCurveInterpolationType::MCIT_Monotone_Cubic:
                return true;
            default:
                return false;
            }
        case RCIM_None:
        default:
            return MythicaInterpolation == EMythicaCurveInterpolationType::MCIT_Invalid;
        }
    }

    /** Converts Unreal interpolation type to the most appropriate Mythica interpolation type. */
    static EMythicaCurveInterpolationType
        TranslateInterpolation(const ERichCurveInterpMode InterpMode)
    {
        switch (InterpMode)
        {
        case RCIM_Linear:
            return EMythicaCurveInterpolationType::MCIT_Linear;
        case RCIM_Constant:
            return EMythicaCurveInterpolationType::MCIT_Constant;
        case RCIM_Cubic:
            return EMythicaCurveInterpolationType::MCIT_Catmull_Rom;
        case RCIM_None:
        default:
            return EMythicaCurveInterpolationType::MCIT_Invalid;
        }
    }


protected:

    /** The delegate used to notify on a change to the curves data. */
    FOnCurveChanged OnCurveChangedDelegate;

protected:

    /** Storing a shared pointer to the data provide to pull from while this curve editor is open. */
    TSharedPtr<ProviderType> DataProvider;

};

#undef LOCTEXT_NAMESPACE
