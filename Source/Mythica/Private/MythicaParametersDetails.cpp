#include "MythicaParametersDetails.h"

#include "MythicaTypes.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SNumericEntryBox.h"

static FMythicaParameters* GetParametersFromHandle(IPropertyHandle& Handle)
{
    TArray<UObject*> Objects;
    Handle.GetOuterObjects(Objects);
    if (Objects.Num() != 1)
    {
        return nullptr;
    }

    return (FMythicaParameters*)Handle.GetValueBaseAddress((uint8*)Objects[0]);
}

static FMythicaParameters* GetParametersFromHandleWeak(TWeakPtr<IPropertyHandle> HandleWeak)
{
    if (!HandleWeak.IsValid())
        return nullptr;

    IPropertyHandle* Handle = HandleWeak.Pin().Get();
    if (!Handle)
        return nullptr;

    return GetParametersFromHandle(*Handle);
}

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
    FMythicaParameters* Parameters = GetParametersFromHandle(StructPropertyHandle.Get());
    if (!Parameters)
    {
        return;
    }

    TWeakPtr<IPropertyHandle> HandleWeak = StructPropertyHandle.ToWeakPtr();
    for (int32 i = 0; i < Parameters->Parameters.Num(); ++i)
    {
        auto GetValue = [HandleWeak, i]()
        {
            FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
            if (!Parameters || !Parameters->Parameters.IsValidIndex(i))
            {
                return 0.0f;
            }

            return Parameters->Parameters[i].Value;
        };

        auto OnValueChanged = [HandleWeak, i](float NewValue)
        {
            FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
            if (!Parameters || !Parameters->Parameters.IsValidIndex(i))
            {
                return;
            }

            Parameters->Parameters[i].Value = NewValue;
        };

        const FString& Name = Parameters->Parameters[i].Name;
        StructBuilder.AddCustomRow(FText::FromString(Name))
            .NameContent()
            [
                SNew(STextBlock)
                    .Text(FText::FromString(Name))
            ]
            .ValueContent()
            [
                SNew(SNumericEntryBox<float>)
                    .Value_Lambda(GetValue)
                    .OnValueChanged_Lambda(OnValueChanged)
            ];
    }
}
