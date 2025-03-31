#pragma once

#include "IPropertyTypeCustomization.h"

struct FMythicaParameterEnumValue;

class FMythicaParametersDetails : public IPropertyTypeCustomization
{
public:
    virtual ~FMythicaParametersDetails() override;

    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
    virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:

    using FReplacementObjectMap = TMap<UObject*, UObject*>;
    void OnObjectsReinstanced(const FReplacementObjectMap& ObjectMap);

private:
    TWeakPtr<IPropertyHandle> HandleWeak;
    TSharedPtr<IPropertyHandle> StructProperty;
    TSharedPtr<IPropertyUtilities> PropUtils;

    // Hold references to avoid garbage collection
    using TEnumOptions = TArray<TSharedPtr<FMythicaParameterEnumValue>>;
    TArray<TSharedPtr<TEnumOptions>> EnumOptionSets;

    bool UsingSlider = false;

    FDelegateHandle OnObjectsReinstancedHandle;
};
