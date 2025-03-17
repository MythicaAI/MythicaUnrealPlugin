

#pragma once

#include "CoreMinimal.h"

#include "Curves/RealCurve.h"

#define LOCTEXT_NAMESPACE "MythicaEditor"

// Disclamer: Houdini helped design this templating system for the CurveEditor

class IMythicaCurveEditor
{
public:

    /** Used so that all curve types can handle a refresh to its contained data. */
    virtual void RefreshCurveKeys() = 0;
};

template<typename ParamType, typename ValueType>
class FMythicaDataProviderBase
{

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
template<typename BaseClass/*, typename DataProvider*/>
class SMythicaCurveEditorBase : public BaseClass, public IMythicaCurveEditor
{

    //using ParserType = DataProvider;
    //using ValueType = typename DataProvider::ValueType;
    //using ParameterType = typename DataProvider::ParameterType;
    //using ParameterWeakPtr = typename CurveParserType::ParameterWeakPtr;
    //using PointType = typename DataProvider::PointType;

protected:

    /**
     * Get data about the curve used by the editor widget.
     *
     * @return Unset optional is only returned if and only if the curve is invalid or if the index
     *         is out of bounds.
     */
    virtual TOptional<int32> GetNumCurveKeys() const = 0;
    virtual TOptional<float> GetCurveKeyPosition(const int32 Index) const = 0;
    //virtual TOptional<ValueType> GetCurveKeyValue(const int32 Index) const = 0;
    virtual TOptional<ERichCurveInterpMode> GetCurveKeyInterpolationType(
        const int32 Index) const = 0;

    void OnCurveChanged()
    {
        
    }

protected:

    /** The delegate used to notify on a change to the curves data. */
    FOnCurveChanged OnCurveChangedDelegate;

};

#undef LOCTEXT_NAMESPACE
