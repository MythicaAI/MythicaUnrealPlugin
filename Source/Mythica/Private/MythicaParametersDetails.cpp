#include "MythicaParametersDetails.h"
#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SNumericEntryBox.h"

TSharedRef<IPropertyTypeCustomization> FMythicaParametersDetails::MakeInstance()
{
    return MakeShareable(new FMythicaParametersDetails);
}

void FMythicaParametersDetails::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    HeaderRow.ShouldAutoExpand(true);
}

void FMythicaParametersDetails::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    // Example of adding a custom numeric entry box
    StructBuilder.AddCustomRow(FText::FromString("CustomField"))
        .NameContent()
        [
            SNew(STextBlock)
                .Text(FText::FromString("Custom Field"))
        ]
        .ValueContent()
        [
            SNew(SNumericEntryBox<float>)
                .Value(TOptional<float>(0.0f)) // Set the default value here
                .OnValueChanged_Lambda([](float NewValue)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("New value: %f"), NewValue);
                    })
        ];

    StructBuilder.AddCustomRow(FText::FromString("Custom Field2"))
        .NameContent()
        [
            SNew(STextBlock)
                .Text(FText::FromString("Custom Field2"))
        ]
        .ValueContent()
        [
            SNew(SNumericEntryBox<float>)
                .Value(TOptional<float>(0.0f)) // Set the default value here
                .OnValueChanged_Lambda([](float NewValue)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("New value: %f"), NewValue);
                    })
        ];

    StructBuilder.AddCustomRow(FText::FromString("Custom Field3"))
        .NameContent()
        [
            SNew(STextBlock)
                .Text(FText::FromString("Custom Field3"))
        ]
        .ValueContent()
        [
            SNew(SNumericEntryBox<float>)
                .Value(TOptional<float>(0.0f)) // Set the default value here
                .OnValueChanged_Lambda([](float NewValue)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("New value: %f"), NewValue);
                    })
        ];
}