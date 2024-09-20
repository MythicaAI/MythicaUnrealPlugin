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
            SNew(SButton)
                .Text(FText::FromString("Regenerate Mesh"))
                .OnClicked_Lambda([ComponentWeak]()
                {
                    if (ComponentWeak.IsValid())
                    {
                        ComponentWeak->RegenerateMesh();
                    }
                    return FReply::Handled();
                })
        ];
}