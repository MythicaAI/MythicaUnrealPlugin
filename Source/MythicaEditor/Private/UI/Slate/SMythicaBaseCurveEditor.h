

#pragma once

#include "CoreMinimal.h"

#include "MythicaTypes.h"
#include "PropertyHandle.h"
#include "Curves/RealCurve.h"

#include "MythicaEditorPrivatePCH.h"

#define LOCTEXT_NAMESPACE MYTHICA_LOCTEXT_NAMESPACE

// Inspired by Houdini's CurveEditor design

template<typename InValueType, typename InParamType, typename InPointType>
class TMythicaBaseDataProvider
{

public:

    using ValueType = InValueType;
    using ParamType = InParamType;
    using PointType = InPointType;

public:

    explicit TMythicaBaseDataProvider(TWeakPtr<IPropertyHandle> InHandle, int32_t InParamIndex)
        : ParamIndex(InParamIndex)
        , ParamHandle(InHandle)
    {
    }

    const TArray<PointType> GetDefaultPoints() const
    {
        TArray<PointType> Defaults = TArray<PointType>();
        if (!ParamHandle.IsValid() || !ParamHandle.Pin()->IsValidHandle())
        {
            return Defaults;
        }

        TSharedPtr<IPropertyHandle> PinnedHandle = ParamHandle.Pin();
        PinnedHandle->EnumerateRawData([this, &Defaults](void* RawData, const int32 /*DataIndex*/, const int32 /*NumDatas*/)
            {
                if (FMythicaParameters* Params = static_cast<FMythicaParameters*>(RawData))
                {
                    Defaults.Append(Params->Parameters[ParamIndex].ValueCurve.DefaultPoints);

                    return false;
                }
                return true;
            });

        return MoveTemp(Defaults);
    }

    const bool ResetToDefaults() const
    {
        if (!ParamHandle.IsValid() || !ParamHandle.Pin()->IsValidHandle())
        {
            return false;
        }

        TSharedPtr<IPropertyHandle> PinnedHandle = ParamHandle.Pin();

        PinnedHandle->NotifyPreChange();
        PinnedHandle->EnumerateRawData([this](void* RawData, const int32 /*DataIndex*/, const int32 /*NumDatas*/)
            {
                if (FMythicaParameters* Params = static_cast<FMythicaParameters*>(RawData))
                {
                    FMythicaParameterCurve& CurveData = Params->Parameters[ParamIndex].ValueCurve;
                    CurveData.Points = CurveData.DefaultPoints;

                    return false;
                }
                return true;
            });

        PinnedHandle->NotifyPostChange(EPropertyChangeType::ArrayRemove);
        PinnedHandle->NotifyFinishedChangingProperties();

        return true;
    }

    bool GetPoint(const int32 Index, PointType& OutPoint) const
    {
        if (!ParamHandle.IsValid() || !ParamHandle.Pin()->IsValidHandle())
        {
            return false;
        }

        ParamHandle.Pin()->EnumerateRawData([this, Index, &OutPoint](void* RawData, const int32 /*DataIndex*/, const int32 /*NumDatas*/)
            {
                if (FMythicaParameters* Params = static_cast<FMythicaParameters*>(RawData))
                {
                    ensure(Params->Parameters[ParamIndex].ValueCurve.Points.IsValidIndex(Index));

                    OutPoint = Params->Parameters[ParamIndex].ValueCurve.Points[Index];

                    return false;
                }
                return true;
            });

        return true;
    }

    /** Gets the number of points the ramp currently has. */
    int32 GetPointCount() const
    {
        if (!ParamHandle.IsValid() || !ParamHandle.Pin()->IsValidHandle())
        {
            return 0;
        }

        TSharedPtr<IPropertyHandle> PinnedHandle = ParamHandle.Pin();

        int32 OutCount = 0;
        PinnedHandle->EnumerateRawData([this, &OutCount](void* RawData, const int32 /*DataIndex*/, const int32 /*NumDatas*/)
            {
                if (FMythicaParameters* Params = static_cast<FMythicaParameters*>(RawData))
                {
                    OutCount = Params->Parameters[ParamIndex].ValueCurve.Points.Num();

                    return false;
                }
                return true;
            });

        return OutCount;
    }

    bool InsertPoint(
            const int32 Index,
            const float Position,
            const ValueType Value,
            const EMythicaCurveInterpType InterpolationType)
    {
        if (!ParamHandle.IsValid() || !ParamHandle.Pin()->IsValidHandle())
        {
            return false;
        }

        TSharedPtr<IPropertyHandle> PinnedHandle = ParamHandle.Pin();

        PinnedHandle->NotifyPreChange();

        PointType NewPoint{Position, Value, InterpolationType};
        PinnedHandle->EnumerateRawData([this, Index, NewPoint](void* RawData, const int32 /*DataIndex*/, const int32 /*NumDatas*/)
            {
                if (FMythicaParameters* Params = static_cast<FMythicaParameters*>(RawData))
                {
                    Params->Parameters[ParamIndex].ValueCurve.Points.Insert(NewPoint, Index);

                    return false;
                }
                return true;
            });

        PinnedHandle->NotifyPostChange(EPropertyChangeType::ArrayRemove);
        PinnedHandle->NotifyFinishedChangingProperties();

        return true;
    }

    bool DeletePoints(const TArrayView<const int32> Indices)
    {
        if (!ParamHandle.IsValid() || !ParamHandle.Pin()->IsValidHandle())
        {
            return false;
        }

        TSharedPtr<IPropertyHandle> PinnedHandle = ParamHandle.Pin();

        PinnedHandle->NotifyPreChange();

        PinnedHandle->EnumerateRawData([this, Indices](void* RawData, const int32 /*DataIndex*/, const int32 /*NumDatas*/)
            {
                if (FMythicaParameters* Params = static_cast<FMythicaParameters*>(RawData))
                {
                    for (int32 Index : Indices)
                    {
                        Params->Parameters[ParamIndex].ValueCurve.Points.RemoveAt(Index);
                    }

                    return false;
                }
                return true;
            });

        PinnedHandle->NotifyPostChange(EPropertyChangeType::ArrayRemove);
        PinnedHandle->NotifyFinishedChangingProperties();

        return true;
    }

    bool DeletePoint(const int32 Index)
    {
        return DeletePoints(MakeArrayView(&Index, 1));
    }

    bool ClearPoints()
    {
        if (!ParamHandle.IsValid() || !ParamHandle.Pin()->IsValidHandle())
        {
            return false;
        }

        TSharedPtr<IPropertyHandle> PinnedHandle = ParamHandle.Pin();

        PinnedHandle->NotifyPreChange();

        PinnedHandle->EnumerateRawData([this](void* RawData, const int32 /*DataIndex*/, const int32 /*NumDatas*/)
        {
            if (FMythicaParameters* Params = static_cast<FMythicaParameters*>(RawData))
            {
                Params->Parameters[ParamIndex].ValueCurve.Points.Empty();

                return false;
            }
            return true;
        });

        PinnedHandle->NotifyPostChange(EPropertyChangeType::ArrayClear);
        PinnedHandle->NotifyFinishedChangingProperties();

        return true;
    }

    TOptional<float> GetPointPosition(const int32 Index) const
    {
        PointType Point;
        if (GetPoint(Index, Point))
        {
            return Point.Pos;
        }

        return TOptional<float>();
    }

    TOptional<ValueType> GetPointValue(const int32 Index) const
    {
        PointType Point;
        if (GetPoint(Index, Point))
        {
            return Point.GetValueWithType<ValueType>();
        }

        return TOptional<ValueType>();
    }

    TOptional<EMythicaCurveInterpType> GetPointInterpolationType(const int32 Index) const
    {
        PointType Point;
        if (GetPoint(Index, Point))
        {
            return Point.InterpType;
        }

        return TOptional<EMythicaCurveInterpType>();
    }

    /** @returns Interpolation if successful, unset optional if unsuccessful. */
    TOptional<ERichCurveInterpMode>
        GetPointRichCurveInterpolationType(const int32 Index) const
    {
        // @TODO: Need better failure handling for this case
        PointType Point;
        if (GetPoint(Index, Point))
        {
            switch (Point.InterpType)
            {
            case EMythicaCurveInterpType::MCIT_Linear:
                return RCIM_Linear;
            case EMythicaCurveInterpType::MCIT_Constant:
                return RCIM_Constant;
            case EMythicaCurveInterpType::MCIT_Bezier:
            case EMythicaCurveInterpType::MCIT_BSpline:
            case EMythicaCurveInterpType::MCIT_Catmull_Rom:
            case EMythicaCurveInterpType::MCIT_Hermite:
            case EMythicaCurveInterpType::MCIT_Monotone_Cubic:
                return RCIM_Cubic;
            case EMythicaCurveInterpType::MCIT_Invalid:
            default:
                return RCIM_None;
            }
        }

        return TOptional<ERichCurveInterpMode>();
    }

private:

    /** The index of the parameter this data provider points to */
    int32_t ParamIndex = -1;

    /** The property that is being modified/viewed */
    TWeakPtr<IPropertyHandle> ParamHandle;

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

public:

    using ProviderType = DataProviderType;
    using ValueType = typename DataProviderType::ValueType;
    using ParamType = typename DataProviderType::ParamType;
    using PointType = typename DataProviderType::PointType;

    /** Should reset the curve data to the data providers defaults. Then we make sure to sync our data. */
    virtual void ResetToDefault()
    {
        if (!DataProvider.IsValid())
        {
            return;
        }

        DataProvider->ResetToDefaults();

        SyncCurveKeys();
    }

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

    /** Syncs the curve data with the data providers. Used when we need to modify curve data externally (Resetting defaults). */
    virtual void SyncCurveKeys() = 0;

    /** We want to check to make sure our internal data matches the curves. */
    void OnCurveChanged()
    {
        if (!DataProvider.IsValid())
        {
            return;
        }

        //UE_LOG(LogMythicaEditor, Warning, TEXT("Curve Updated %s"), *DataProvider->ToString());

        const int32 NumPoints = DataProvider->GetPointCount();
        const TOptional<int32> NumCurveKeys = GetNumCurveKeys();

        if (!NumCurveKeys.IsSet())
        {
            return;
        }

        DataProvider->ClearPoints();

        for (int Index = 0; Index < NumCurveKeys.GetValue(); Index++)
        {
            float Pos = GetCurveKeyPosition(Index).GetValue();
            ValueType Value = GetCurveKeyValue(Index).GetValue();
            ERichCurveInterpMode Interp = GetCurveKeyInterpolationType(Index).GetValue();

            DataProvider->InsertPoint(Index, Pos, Value, TranslateInterpolation(Interp));
        }

        //// Note! The EPropertyChangeType is (unintuitively) always EPropertyChangeType::ValueSet
        //const bool bIsAddingPoints = NumCurveKeys.GetValue() > NumPoints;
        //const bool bIsDeletingPoints = NumCurveKeys.GetValue() < NumPoints;

        //// To report what indicies have changed
        //TArray<int32> ModifiedIndicies;

        //// If we are deleting points we want to make sure we loop throught the larger collection
        //uint32 LoopRange = ((bIsDeletingPoints) ? NumPoints : NumCurveKeys.GetValue()) - 1;
        //for (uint32 Index = 0; Index < LoopRange; Index++)
        //{
        //    if (bIsDeletingPoints)
        //    {
        //        // Check for deleted indicies
        //        PointType Point = DataProvider->GetPoint(Index);

        //    }
        //    else if (bIsAddingPoints)
        //    {
        //        // Check for added indicies
        //    }
        //    else
        //    {
        //        // Check to see if interpolation type changed
        //    }
        //}

        OnCurveChangedDelegate.ExecuteIfBound();
    }

protected:

    /**
     * Since there are fewer Unreal interpolation types than Houdini interpolation types, we map
     * multiple of our Mythica interpolation types to Unreal's cubic interpolation type.
     */
    static bool IsInterpolationEquivalent(const ERichCurveInterpMode UnrealInterpolation, const EMythicaCurveInterpType MythicaInterpolation)
    {
        switch (UnrealInterpolation)
        {
        case ERichCurveInterpMode::RCIM_Linear:
            return MythicaInterpolation == EMythicaCurveInterpType::MCIT_Linear;
        case ERichCurveInterpMode::RCIM_Constant:
            return MythicaInterpolation == EMythicaCurveInterpType::MCIT_Constant;
        case ERichCurveInterpMode::RCIM_Cubic:
            switch (MythicaInterpolation)
            {
            case EMythicaCurveInterpType::MCIT_Bezier:
            case EMythicaCurveInterpType::MCIT_BSpline:
            case EMythicaCurveInterpType::MCIT_Catmull_Rom:
            case EMythicaCurveInterpType::MCIT_Hermite:
            case EMythicaCurveInterpType::MCIT_Monotone_Cubic:
                return true;
            default:
                return false;
            }
        case ERichCurveInterpMode::RCIM_None:
        default:
            return MythicaInterpolation == EMythicaCurveInterpType::MCIT_Invalid;
        }
    }

    /** Converts Unreal interpolation type to the most appropriate Mythica interpolation type. */
    static EMythicaCurveInterpType TranslateInterpolation(const ERichCurveInterpMode InterpMode)
    {
        switch (InterpMode)
        {
        case ERichCurveInterpMode::RCIM_Linear:
            return EMythicaCurveInterpType::MCIT_Linear;
        case ERichCurveInterpMode::RCIM_Constant:
            return EMythicaCurveInterpType::MCIT_Constant;
        case ERichCurveInterpMode::RCIM_Cubic:
            return EMythicaCurveInterpType::MCIT_Catmull_Rom;
        case ERichCurveInterpMode::RCIM_None:
        default:
            return EMythicaCurveInterpType::MCIT_Invalid;
        }
    }

    static ERichCurveInterpMode TranslateInterpolation(const EMythicaCurveInterpType InterpType)
    {
        switch (InterpType)
        {
        case EMythicaCurveInterpType::MCIT_Linear:
            return ERichCurveInterpMode::RCIM_Linear;
        case EMythicaCurveInterpType::MCIT_Constant:
            return ERichCurveInterpMode::RCIM_Constant;
        case EMythicaCurveInterpType::MCIT_Bezier:
        case EMythicaCurveInterpType::MCIT_BSpline:
        case EMythicaCurveInterpType::MCIT_Catmull_Rom:
        case EMythicaCurveInterpType::MCIT_Hermite:
        case EMythicaCurveInterpType::MCIT_Monotone_Cubic:
            return ERichCurveInterpMode::RCIM_Cubic;
        case EMythicaCurveInterpType::MCIT_Invalid:
        default:
            return ERichCurveInterpMode::RCIM_None;
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
