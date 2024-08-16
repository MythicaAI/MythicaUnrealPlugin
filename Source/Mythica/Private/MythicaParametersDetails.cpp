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
    for (int32 ParamIndex = 0; ParamIndex < Parameters->Parameters.Num(); ++ParamIndex)
    {
        const FMythicaParameter& Parameter = Parameters->Parameters[ParamIndex];

        if (const FMythicaParameterFloat* FloatParameter = Parameter.Value.TryGet<FMythicaParameterFloat>())
        {
            if (FloatParameter->Values.Num() != 1)
            {
                continue;
            }

            auto Value = [=]()
            {
                FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                return Parameters ? Parameters->Parameters[ParamIndex].Value.Get<FMythicaParameterFloat>().Values[0] : 0.0f;
            };

            auto OnValueCommitted = [=](float NewValue, ETextCommit::Type InCommitType)
            {
                FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                if (Parameters)
                    Parameters->Parameters[ParamIndex].Value.Get<FMythicaParameterFloat>().Values[0] = NewValue;
            };

            StructBuilder.AddCustomRow(FText::FromString(Parameter.Label))
                .NameContent()
                [
                    SNew(STextBlock)
                        .Text(FText::FromString(Parameter.Label))
                ]
                .ValueContent()
                [
                    SNew(SNumericEntryBox<float>)
                        .Value_Lambda(Value)
                        .OnValueCommitted_Lambda(OnValueCommitted)
                ];
        }
        else if (const FMythicaParameterInt* IntParameter = Parameter.Value.TryGet<FMythicaParameterInt>())
        {
            if (IntParameter->Values.Num() != 1)
            {
                continue;
            }

            auto Value = [=]()
            {
                FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                return Parameters ? Parameters->Parameters[ParamIndex].Value.Get<FMythicaParameterInt>().Values[0] : 0;
            };

            auto OnValueCommitted = [=](int NewValue, ETextCommit::Type InCommitType)
            {
                FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                if (Parameters)
                    Parameters->Parameters[ParamIndex].Value.Get<FMythicaParameterInt>().Values[0] = NewValue;
            };

            StructBuilder.AddCustomRow(FText::FromString(Parameter.Label))
                .NameContent()
                [
                    SNew(STextBlock)
                        .Text(FText::FromString(Parameter.Label))
                ]
                .ValueContent()
                [
                    SNew(SNumericEntryBox<int>)
                        .Value_Lambda(Value)
                        .OnValueCommitted_Lambda(OnValueCommitted)
                ];
        }
        else if (const FMythicaParameterBool* BoolParameter = Parameter.Value.TryGet<FMythicaParameterBool>())
        {
            auto IsChecked = [=]()
            {
                FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                bool Value = Parameters ? Parameters->Parameters[ParamIndex].Value.Get<FMythicaParameterBool>().Value : false;
                return Value ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
            };

            auto OnCheckStateChanged = [=](ECheckBoxState NewState)
            {
                if (NewState == ECheckBoxState::Undetermined)
                    return;

                FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                if (Parameters)
                    Parameters->Parameters[ParamIndex].Value.Get<FMythicaParameterBool>().Value = (NewState == ECheckBoxState::Checked);
            };

            StructBuilder.AddCustomRow(FText::FromString(Parameter.Label))
                .NameContent()
                [
                    SNew(STextBlock)
                        .Text(FText::FromString(Parameter.Label))
                ]
                .ValueContent()
                [
                    SNew(SCheckBox)
                        .IsChecked_Lambda(IsChecked)
                        .OnCheckStateChanged_Lambda(OnCheckStateChanged)
                ];
        }
        else if (const FMythicaParameterString* StringParameter = Parameter.Value.TryGet<FMythicaParameterString>())
        {
            auto Text = [=]()
            {
                FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                return Parameters ? FText::FromString(Parameters->Parameters[ParamIndex].Value.Get<FMythicaParameterString>().Value) : FText();
            };

            auto OnTextCommitted = [=](const FText& InText, ETextCommit::Type InCommitType)
            {
                FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                if (Parameters)
                    Parameters->Parameters[ParamIndex].Value.Get<FMythicaParameterString>().Value = InText.ToString();
            };

            StructBuilder.AddCustomRow(FText::FromString(Parameter.Label))
                .NameContent()
                [
                    SNew(STextBlock)
                        .Text(FText::FromString(Parameter.Label))
                ]
                .ValueContent()
                [
                    SNew(SEditableTextBox)
                        .Text_Lambda(Text)
                        .OnTextCommitted_Lambda(OnTextCommitted)
                ];
        }
    }
}
