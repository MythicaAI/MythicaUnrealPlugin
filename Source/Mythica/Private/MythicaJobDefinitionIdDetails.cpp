#include "MythicaJobDefinitionIdDetails.h"
#include "DetailWidgetRow.h"
#include "SSearchableComboBox.h"
#include "PropertyHandle.h"

#include "MythicaEditorSubsystem.h"

TSharedRef<IPropertyTypeCustomization> FMythicaJobDefinitionIdDetails::MakeInstance()
{
    return MakeShareable(new FMythicaJobDefinitionIdDetails);
}

void FMythicaJobDefinitionIdDetails::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    // Gather options
    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    TArray<FMythicaJobDefinition> JobDefinitions = MythicaEditorSubsystem->GetJobDefinitionList("houdini_generate_mesh");

    for (const FMythicaJobDefinition& JobDefinition : JobDefinitions)
    {
        Options.Add(MakeShared<FString>(JobDefinition.Name));
        JobDefIds.Add(JobDefinition.JobDefId);
    }

    // Create widget
    TSharedPtr<IPropertyHandle> StringFieldHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMythicaJobDefinitionId, JobDefId));

    HeaderRow.NameContent()
    [
        StructPropertyHandle->CreatePropertyNameWidget(FText::FromString(TEXT("Tool")))
    ]
    .ValueContent()
    [
        SNew(SSearchableComboBox)
            .OptionsSource(&Options)
            .OnSelectionChanged_Lambda([this, StringFieldHandle](TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
            {
                int32 SelectedIndex = Options.IndexOfByKey(NewValue);
                if (Options.IsValidIndex(SelectedIndex))
                {
                    OnSelectionChanged(JobDefIds[SelectedIndex], StringFieldHandle);
                }
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
                        return FText::FromString(GetSelectedOption(StringFieldHandle));
                    })
            ]
    ];
}

void FMythicaJobDefinitionIdDetails::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{

}

void FMythicaJobDefinitionIdDetails::OnSelectionChanged(const FString& NewValue, TSharedPtr<IPropertyHandle> PropertyHandle)
{
    PropertyHandle->SetValue(*NewValue);
}

FString FMythicaJobDefinitionIdDetails::GetSelectedOption(TSharedPtr<IPropertyHandle> PropertyHandle) const
{
    FString CurrentValue;
    if (PropertyHandle->GetValue(CurrentValue) == FPropertyAccess::Success)
    {
        int32 SelectedIndex = JobDefIds.IndexOfByKey(CurrentValue);
        if (JobDefIds.IsValidIndex(SelectedIndex))
        {
            return *Options[SelectedIndex];
        }
    }

    return FString(TEXT(""));
}