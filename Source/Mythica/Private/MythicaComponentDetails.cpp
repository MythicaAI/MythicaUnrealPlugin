#include "MythicaComponentDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Mythica.h"
#include "MythicaComponent.h"
#include "MythicaDeveloperSettings.h"
#include "SSearchableComboBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "Mythica"

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
    PopulateToolOptions();

    // Create widget
    IDetailCategoryBuilder& MyCategory = DetailBuilder.EditCategory("Mythica", FText::FromString("Mythica"), ECategoryPriority::Important);

    MyCategory.AddCustomRow(FText::FromString("RegenerateMeshRow"))
        .WholeRowContent()
        [
            SNew(SVerticalBox)
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
                                            check(OptionData.IsValidIndex(SelectedIndex));
                                            const FMythicaToolOptionData& Data = OptionData[SelectedIndex];

                                            if (Data.JobDefId.IsEmpty())
                                            {
                                                FMythicaModule& MythicaModule = FModuleManager::GetModuleChecked<FMythicaModule>("Mythica");
                                                MythicaModule.OpenPackageManager();
                                            }
                                            else
                                            {
                                                SelectTool(Data.JobDefId, ComponentWeak);
                                            }
                                        })
                                        .OnGenerateWidget_Lambda([this](TSharedPtr<FString> InOption)
                                        {
                                            int32 SelectedIndex = Options.IndexOfByKey(InOption);
                                            check(OptionData.IsValidIndex(SelectedIndex));
                                            const FMythicaToolOptionData& Data = OptionData[SelectedIndex];

                                            if (Data.JobDefId.IsEmpty())
                                            {
                                                return SNew(SHorizontalBox)
                                                    + SHorizontalBox::Slot()
                                                    .FillWidth(1.0f)
                                                    .HAlign(HAlign_Center)
                                                    .VAlign(VAlign_Center)
                                                    .Padding(FMargin(0.0f, 0.0f, 3.0f, 0.0f))
                                                    [
                                                        SNew(STextBlock)
                                                            .Text(FText::FromString("+ Add favorites"))
                                                            .ColorAndOpacity(FSlateColor(FLinearColor::White))
                                                    ];
                                            }

                                            FSlateFontInfo TitleFontInfo = IDetailLayoutBuilder::GetDetailFont();
                                            FSlateFontInfo SubtitleFontInfo = IDetailLayoutBuilder::GetDetailFont();
                                            TitleFontInfo.Size += 3;

                                            return SNew(SHorizontalBox)
                                                + SHorizontalBox::Slot()
                                                .AutoWidth()
                                                .Padding(FMargin(0.0f, 0.0f, 3.0f, 0.0f))
                                                [
                                                    SNew(SImage)
                                                        .Image(FAppStyle::GetBrush("GenericWhiteBox"))
                                                        .ColorAndOpacity(FLinearColor(0.0f, 0.25f, 0.0f))
                                                        .DesiredSizeOverride(FVector2D(3.0f, 40.0f))
                                                ]
                                                + SHorizontalBox::Slot()
                                                .FillWidth(1.0f)
                                                .Padding(FMargin(5.0f, 0.0f, 5.0f, 0.0f))
                                                .VAlign(VAlign_Center)
                                                [
                                                    SNew(SVerticalBox)
                                                        + SVerticalBox::Slot()
                                                        .AutoHeight()
                                                        [
                                                            SNew(STextBlock)
                                                                .Text(FText::FromString(*Data.Name))
                                                                .Font(TitleFontInfo)
                                                                .ColorAndOpacity(FSlateColor(FLinearColor::White))
                                                        ]
                                                        + SVerticalBox::Slot()
                                                        .AutoHeight()
                                                        [
                                                            SNew(STextBlock)
                                                                .Text(FText::FromString(FString::Printf(TEXT("%s - %s"), *Data.Owner, *Data.AssetName)))
                                                                .Font(SubtitleFontInfo)
                                                                .ColorAndOpacity(FSlateColor(FLinearColor::Gray))
                                                                .Visibility(!Data.AssetName.IsEmpty() ? EVisibility::Visible : EVisibility::Hidden)
                                                        ]
                                                ]
                                                + SHorizontalBox::Slot()
                                                .AutoWidth()
                                                .Padding(FMargin(5.0f))
                                                .HAlign(HAlign_Right)
                                                .VAlign(VAlign_Center)
                                                [
                                                    SNew(STextBlock)
                                                        .Text(FText::FromString(FString::Printf(TEXT("v%s"), *Data.Version)))
                                                        .ColorAndOpacity(FSlateColor(FLinearColor::Gray))
                                                        .Visibility(!Data.Version.IsEmpty() ? EVisibility::Visible : EVisibility::Hidden)
                                                ];
                                        })
                                        .Content()
                                        [
                                            SNew(STextBlock)
                                                .Text_Lambda([this, ComponentWeak]() -> FText
                                                {
                                                    return FText::FromString(GetComboBoxDisplayString(ComponentWeak));
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
                            const UMythicaDeveloperSettings* Settings = GetDefault<UMythicaDeveloperSettings>();
                            if (Settings->GetAPIKey().IsEmpty())
                            {
                                return FText::FromString("Missing API Key");
                            }
                            else
                            {
                                return FText::FromString("Failed to connect to Mythica service");
                            }
                        })
                        .ColorAndOpacity(FLinearColor::Red)
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
                        .Text_Lambda([ComponentWeak]()
                        {
                            FText Error = FText::FromString("Unknown error");
                            if (ComponentWeak.IsValid() && !ComponentWeak->GetJobMessage().IsEmpty())
                            {
                                Error = ComponentWeak->GetJobMessage();
                            }
                            return FText::Format(FText::FromString("Job Failed: {0}"), Error);
                        })
                        .ColorAndOpacity(FLinearColor::Red)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(FMargin(0.0f, 0.0f, 0.0f, 5.0f))
                .HAlign(HAlign_Center)
                [
                    SNew(SHorizontalBox)
                        .Visibility_Lambda([ComponentWeak]()
                        {
                            if (ComponentWeak.IsValid() && ComponentWeak->Source.IsValid())
                            {
                                UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
                                FMythicaJobDefinition LatestDefinition = MythicaEditorSubsystem->GetJobDefinitionLatest(ComponentWeak->Source);

                                if (ComponentWeak->Source.Version < LatestDefinition.Source.Version
                                    || ComponentWeak->Source.Version == LatestDefinition.Source.Version && ComponentWeak->JobDefId.JobDefId != LatestDefinition.JobDefId)
                                {
                                    return EVisibility::Visible;
                                }
                            }

                            return EVisibility::Collapsed;
                        })
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .VAlign(VAlign_Center)
                        [
                            SNew(STextBlock)
                                .Text(FText::FromString(TEXT("New version of tool available")))
                                        .ColorAndOpacity(FLinearColor::Yellow)
                        ]
                        + SHorizontalBox::Slot()
                        .AutoWidth()
                        .Padding(FMargin(5.0f, 0.0f, 0.0f, 0.0f))
                        .VAlign(VAlign_Center)
                        [
                            SNew(SButton)
                                .Text(FText::FromString(TEXT("Update")))
                                .OnClicked_Lambda([this, ComponentWeak]() -> FReply
                                {
                                    if (ComponentWeak.IsValid() && ComponentWeak->Source.IsValid())
                                    {
                                        UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
                                        FMythicaJobDefinition LatestDefinition = MythicaEditorSubsystem->GetJobDefinitionLatest(ComponentWeak->Source);

                                        SelectTool(LatestDefinition.JobDefId, ComponentWeak);
                                    }
                                        
                                    return FReply::Handled();
                                })
                        ]
                ]
        ];
}

void FMythicaComponentDetails::PopulateToolOptions()
{
    UMythicaEditorSubsystem* MythicaEditorSubsystem = GEditor->GetEditorSubsystem<UMythicaEditorSubsystem>();
    TArray<FMythicaJobDefinition> JobDefinitions = MythicaEditorSubsystem->GetJobDefinitionList("houdini::/mythica/generate_mesh");

    for (const FMythicaJobDefinition& JobDefinition : JobDefinitions)
    {
        FMythicaToolOptionData Data;
        Data.JobDefId = JobDefinition.JobDefId;
        Data.Name = JobDefinition.Name;
        Data.Owner = JobDefinition.SourceAssetOwner;
        Data.AssetName = JobDefinition.SourceAssetName;
        Data.Version = JobDefinition.Source.Version.ToString();

        FString SearchString = FString::Printf(TEXT("%s %s %s"), *Data.Owner, *Data.AssetName, *Data.Name);

        Options.Add(MakeShared<FString>(SearchString));
        OptionData.Add(Data);
    }

    Options.Add(MakeShared<FString>("Add new favorites"));
    OptionData.Add({});
}

void FMythicaComponentDetails::SelectTool(const FString& JobDefId, TWeakObjectPtr<class UMythicaComponent> ComponentWeak)
{
    if (ComponentWeak.IsValid())
    {
        const FScopedTransaction Transaction(LOCTEXT("MythicaSelectTool", "Select Tool"));
        ComponentWeak->Modify();

        ComponentWeak->JobDefId.JobDefId = JobDefId;

        FPropertyChangedEvent PropertyChangedEvent(
            FindFieldChecked<FProperty>(UMythicaComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(UMythicaComponent, JobDefId)));
        ComponentWeak->PostEditChangeProperty(PropertyChangedEvent);
    }
}

FString FMythicaComponentDetails::GetComboBoxDisplayString(TWeakObjectPtr<class UMythicaComponent> ComponentWeak) const
{
    if (ComponentWeak.IsValid() && !ComponentWeak->JobDefId.JobDefId.IsEmpty())
    {
        if (ComponentWeak->Source.IsValid())
        {
            return FString::Printf(TEXT("%s - v%s"), *ComponentWeak->ToolName, *ComponentWeak->Source.Version.ToString());
        }
        else
        {
            return ComponentWeak->ToolName;
        }
    }

    return FString(TEXT("Select a tool..."));
}