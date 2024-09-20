#pragma once

#include "IDetailCustomization.h"

class FMythicaJobDefinitionIdDetails : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
    TArray<TSharedPtr<FString>> Options;
    TArray<FString> JobDefIds;

    void OnSelectionChanged(const FString& NewValue, TSharedPtr<IPropertyHandle> PropertyHandle);
    TSharedPtr<FString> GetSelectedOption(TSharedPtr<IPropertyHandle> PropertyHandle) const;
};