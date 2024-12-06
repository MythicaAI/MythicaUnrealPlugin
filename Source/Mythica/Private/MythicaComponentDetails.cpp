#include "MythicaComponentDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "MythicaComponent.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"

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
                        .HeightOverride(3.0f)
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
                .Padding(FMargin(0, 5, 0, 5))
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot()
                        .HAlign(HAlign_Center)
                        .VAlign(VAlign_Center)
                        .FillWidth(0.5f)
                        [
                            SNew(SButton)
                                .Text(FText::FromString("Generate"))
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
                        + SHorizontalBox::Slot()
                        .HAlign(HAlign_Center)
                        .VAlign(VAlign_Center)
                        .FillWidth(0.5f)
                        [
                            SNew(SVerticalBox)
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(FMargin(0, 5, 0, 0))
                                .HAlign(HAlign_Center)
                                [
                                    SNew(STextBlock)
                                        .Text_Lambda([ComponentWeak]()
                                        {
                                            if (ComponentWeak.IsValid())
                                            {
                                                EMythicaJobState State = ComponentWeak->GetJobState();
                                                if (State != EMythicaJobState::Invalid && State != EMythicaJobState::Completed)
                                                {
                                                    FString StateString = StaticEnum<EMythicaJobState>()->GetNameStringByValue(static_cast<int64>(State));
                                                    return FText::FromString(StateString);
                                                }
                                            }
                                            return FText::FromString("");
                                        })
                                ]
                        ]
                ]
        ];
}