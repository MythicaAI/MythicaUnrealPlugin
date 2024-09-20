#include "MythicaJobDefinitionIdDetails.h"
#include "DetailWidgetRow.h"
#include "SSearchableComboBox.h"
#include "PropertyHandle.h"

TSharedRef<IPropertyTypeCustomization> FMythicaJobDefinitionIdDetails::MakeInstance()
{
    return MakeShareable(new FMythicaJobDefinitionIdDetails);
}

void FMythicaJobDefinitionIdDetails::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    TSharedPtr<IPropertyHandle> StringFieldHandle = StructPropertyHandle->GetChildHandle("JobDefId");

    HeaderRow.NameContent()
    [
        StructPropertyHandle->CreatePropertyNameWidget()
    ]
    .ValueContent()
    [
        SNew(SSearchableComboBox)
            .OptionsSource(&ComboBoxOptions)
            .OnSelectionChanged_Lambda([this, StringFieldHandle](TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
            {
                OnSelectionChanged(NewValue, StringFieldHandle);
            })
            .OnGenerateWidget_Lambda([](TSharedPtr<FString> InOption)
            {
                return SNew(STextBlock).Text(FText::FromString(*InOption));
            })
            .Content()
            [
                SNew(STextBlock)
                    .Text_Lambda([this, StringFieldHandle]() -> FText
                    {
                        return FText::FromString(*GetSelectedOption(StringFieldHandle));
                    })
            ]
        ];
}

void FMythicaJobDefinitionIdDetails::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{

}

void FMythicaJobDefinitionIdDetails::OnSelectionChanged(TSharedPtr<FString> NewValue, TSharedPtr<IPropertyHandle> PropertyHandle)
{
    if (NewValue.IsValid() && PropertyHandle->IsValidHandle())
    {
        PropertyHandle->SetValue(*NewValue);
    }
}

TSharedPtr<FString> FMythicaJobDefinitionIdDetails::GetSelectedOption(TSharedPtr<IPropertyHandle> PropertyHandle) const
{
    FString CurrentValue;
    if (PropertyHandle->GetValue(CurrentValue) == FPropertyAccess::Success)
    {
        for (const TSharedPtr<FString>& Option : ComboBoxOptions)
        {
            if (*Option == CurrentValue)
            {
                return Option;
            }
        }
    }

    return ComboBoxOptions[0];
}