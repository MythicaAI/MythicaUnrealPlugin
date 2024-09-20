#include "MythicaJobDefinitionIdDetails.h"

#include "MythicaTypes.h"

TSharedRef<IPropertyTypeCustomization> FMythicaJobDefinitionIdDetails::MakeInstance()
{
    return MakeShareable(new FMythicaJobDefinitionIdDetails);
}

void FMythicaJobDefinitionIdDetails::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{

}

void FMythicaJobDefinitionIdDetails::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{

}
