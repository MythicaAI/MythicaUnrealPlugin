#include "MythicaComponentDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IDetailCustomization> FMythicaComponentDetails::MakeInstance()
{
    return MakeShareable(new FMythicaComponentDetails);
}

void FMythicaComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    IDetailCategoryBuilder& MyCategory = DetailBuilder.EditCategory("CustomCategory", FText::FromString("My Custom Category"), ECategoryPriority::Important);

    MyCategory.AddCustomRow(FText::FromString("Custom Text"))
        .NameContent()
        [
            SNew(STextBlock).Text(FText::FromString("My Label"))
        ]
        .ValueContent()
        [
            SNew(STextBlock).Text(FText::FromString("My Custom Value"))
        ];
}