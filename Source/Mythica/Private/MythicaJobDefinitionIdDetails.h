#pragma once

#include "IDetailCustomization.h"

class FMythicaJobDefinitionIdDetails : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	virtual void CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
    TArray<TSharedPtr<FString>> ComboBoxOptions = { MakeShareable(new FString("Option1")), MakeShareable(new FString("Option2")), MakeShareable(new FString("Option3")) };

    void OnSelectionChanged(TSharedPtr<FString> NewValue, TSharedPtr<IPropertyHandle> PropertyHandle);
    TSharedPtr<FString> GetSelectedOption(TSharedPtr<IPropertyHandle> PropertyHandle) const;
};