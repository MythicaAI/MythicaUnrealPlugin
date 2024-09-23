#include "MythicaComponentDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "MythicaComponent.h"
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
            SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                .FillWidth(0.5f)
                [
                    SNew(SButton)
                        .Text(FText::FromString("Regenerate Mesh"))
                        .IsEnabled_Lambda([ComponentWeak]()
                        {
                            if (ComponentWeak.IsValid())
                            {
                                EMythicaJobState State = ComponentWeak->GetJobState();
                                return State == EMythicaJobState::Invalid ||
                                       State == EMythicaJobState::Failed ||
                                       State == EMythicaJobState::Completed;
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
        ];
}