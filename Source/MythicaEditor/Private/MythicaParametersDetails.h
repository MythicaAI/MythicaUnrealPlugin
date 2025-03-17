#pragma once

#include "IPropertyTypeCustomization.h"

struct FMythicaParameterEnumValue;

class FMythicaParametersDetails : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
    TWeakPtr<IPropertyHandle> HandleWeak;

    // Hold references to avoid garbage collection
    using TEnumOptions = TArray<TSharedPtr<FMythicaParameterEnumValue>>;
    TArray<TSharedPtr<TEnumOptions>> EnumOptionSets;

    bool UsingSlider = false;
};
