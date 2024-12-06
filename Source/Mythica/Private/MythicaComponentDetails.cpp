#include "MythicaComponentDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "MythicaComponent.h"
#include "SSearchableComboBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"

const float ProgressBarHeight = 3.0f;

TSharedRef<IDetailCustomization> FMythicaComponentDetails::MakeInstance()
{
    return MakeShareable(new FMythicaComponentDetails);
}

void FMythicaComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
    DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);
    if (ObjectsBeingCustomized.Num() != 1)
    {
        return;
    }

    UMythicaComponent* Component = Cast<UMythicaComponent>(ObjectsBeingCustomized[0].Get());
    TWeakObjectPtr<class UMythicaComponent> ComponentWeak = Component;

    // Gather options
    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    TArray<FMythicaJobDefinition> JobDefinitions = MythicaEditorSubsystem->GetJobDefinitionList("houdini::/mythica/generate_mesh");

    for (const FMythicaJobDefinition& JobDefinition : JobDefinitions)
    {
        Options.Add(MakeShared<FString>(JobDefinition.Name));
        JobDefIds.Add(JobDefinition.JobDefId);
    }

    // Create widget
    IDetailCategoryBuilder& MyCategory = DetailBuilder.EditCategory("Mythica", FText::FromString("Mythica"), ECategoryPriority::Important);

    MyCategory.AddCustomRow(FText::FromString("RegenerateMeshRow"))
        .WholeRowContent()
        [
            SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 10, 0, 10)
                [
                    SNew(STextBlock)
                        .Justification(ETextJustify::Center)
                        .Visibility_Lambda([]()
                        {
                            UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
                            if (MythicaEditorSubsystem->GetSessionState() != EMythicaSessionState::SessionCreated)
                            {
                                return EVisibility::Visible;
                            }
                            else
                            {
                                return EVisibility::Collapsed;
                            }
                        })
                        .Text_Lambda([]()
                        {
                            return FText::FromString("Failed to connect to Mythica service");
                        })
                        .ColorAndOpacity(FLinearColor::Red)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 10, 0, 10)
                [
                    SNew(STextBlock)
                        .Justification(ETextJustify::Center)
                        .Visibility_Lambda([ComponentWeak]()
                        {
                            if (ComponentWeak.IsValid())
                            {
                                return ComponentWeak->CanRegenerateMesh() ? EVisibility::Collapsed : EVisibility::Visible;
                            }
                            return EVisibility::Collapsed;
                        })
                        .Text_Lambda([]()
                        {
                            return FText::FromString("Not supported in actor blueprint editor");
                        })
                        .ColorAndOpacity(FLinearColor::Red)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SBox)
                        .HeightOverride(ProgressBarHeight)
                        [
                            SNew(SProgressBar)
                                .Percent_Lambda([ComponentWeak]()
                                {
                                    if (ComponentWeak.IsValid())
                                    {
                                        return ComponentWeak->JobProgressPercent();
                                    }
                                    return 0.0f;
                                })
                                .Visibility_Lambda([ComponentWeak]()
                                {
                                    if (ComponentWeak.IsValid())
                                    {
                                        if (ComponentWeak->IsJobProcessing())
                                        {
                                            return EVisibility::Visible;
                                        }
                                    }
                                    return EVisibility::Hidden;
                                })
                        ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0, 5, 0, 5 + ProgressBarHeight))
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .HAlign(HAlign_Center)
                        .VAlign(VAlign_Center)
                        .AutoWidth()
                        [
                            SNew(SBox)
                                .WidthOverride(150.0f)
                                [
                                    SNew(SButton)
                                        .HAlign(HAlign_Center)
                                        .VAlign(VAlign_Center)
                                        .Text_Lambda([ComponentWeak]()
                                        {
                                            if (ComponentWeak.IsValid())
                                            {
                                                if (ComponentWeak->IsJobProcessing())
                                                {
                                                    EMythicaJobState State = ComponentWeak->GetJobState();
                                                    FString StateString = StaticEnum<EMythicaJobState>()->GetNameStringByValue(static_cast<int64>(State));
                                                    StateString.Append("...");
                                                    return FText::FromString(StateString);
                                                }
                                            }
                                            return FText::FromString("Generate");
                                        })
                                        .IsEnabled_Lambda([ComponentWeak]()
                                        {
                                            UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
                                            if (MythicaEditorSubsystem->GetSessionState() != EMythicaSessionState::SessionCreated)
                                            {
                                                return false;
                                            }

                                            if (ComponentWeak.IsValid())
                                            {
                                                if (!ComponentWeak->CanRegenerateMesh())
                                                {
                                                    return false;
                                                }

                                                if (ComponentWeak->JobDefId.JobDefId.IsEmpty())
                                                {
                                                    return false;
                                                }

                                                return !ComponentWeak->IsJobProcessing();
                                            }
                                            return false;
                                        })
                                        .OnClicked_Lambda([ComponentWeak]()
                                        {
                                            if (ComponentWeak.IsValid())
                                            {
                                                ComponentWeak->RegenerateMesh();
                                            }
                                            return FReply::Handled();
                                        })
                                ]
                        ]
                        + SHorizontalBox::Slot()
                        .HAlign(HAlign_Fill)
                        .VAlign(VAlign_Center)
                        .FillWidth(1.0f)
                        .Padding(FMargin(10.0f, 0.0f))
                        [
                            SNew(SBox)
                                [
                                    SNew(SSearchableComboBox)
                                        .OptionsSource(&Options)
                                        .OnSelectionChanged_Lambda([this, ComponentWeak](TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
                                        {
                                            int32 SelectedIndex = Options.IndexOfByKey(NewValue);
                                            if (Options.IsValidIndex(SelectedIndex))
                                            {
                                                OnSelectionChanged(JobDefIds[SelectedIndex], ComponentWeak);
                                            }
                                        })
                                        .OnGenerateWidget_Lambda([](TSharedPtr<FString> InOption)
                                        {
                                            return SNew(STextBlock).Text(FText::FromString(*InOption));
                                        })
                                        .Content()
                                        [
                                            SNew(STextBlock)
                                                .Text_Lambda([this, ComponentWeak]() -> FText
                                                {
                                                    return FText::FromString(GetSelectedOption(ComponentWeak));
                                                })
                                        ]
                                ]
                        ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0.0f, 0.0f, 0.0f, 5.0f))
                [
                    SNew(STextBlock)
                        .Justification(ETextJustify::Center)
                        .Visibility_Lambda([ComponentWeak]()
                        {
                            if (ComponentWeak.IsValid())
                            {
                                EMythicaJobState State = ComponentWeak->GetJobState();
                                if (State == EMythicaJobState::Failed)
                                {
                                    return EVisibility::Visible;
                                }
                            }

                            return EVisibility::Collapsed;
                        })
                        .Text_Lambda([]()
                        {
                            return FText::FromString("Job Failed: Unknown error");
                        })
                        .ColorAndOpacity(FLinearColor::Red)
                ]
        ];
}

void FMythicaComponentDetails::OnSelectionChanged(const FString& NewValue, TWeakObjectPtr<class UMythicaComponent> ComponentWeak)
{
    if (ComponentWeak.IsValid())
    {
        ComponentWeak->JobDefId.JobDefId = NewValue;

        FPropertyChangedEvent PropertyChangedEvent(
            FindFieldChecked<FProperty>(UMythicaComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(UMythicaComponent, JobDefId)));
        ComponentWeak->PostEditChangeProperty(PropertyChangedEvent);
    }
}

FString FMythicaComponentDetails::GetSelectedOption(TWeakObjectPtr<class UMythicaComponent> ComponentWeak) const
{
    if (ComponentWeak.IsValid())
    {
        int32 SelectedIndex = JobDefIds.IndexOfByKey(ComponentWeak->JobDefId.JobDefId);
        if (JobDefIds.IsValidIndex(SelectedIndex))
        {
            return *Options[SelectedIndex];
        }
    }

    return FString(TEXT("Select a tool..."));
}