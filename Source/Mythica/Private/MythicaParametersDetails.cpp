#include "MythicaParametersDetails.h"

#include "MythicaTypes.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Text/SMultiLineEditableText.h"
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

        TSharedRef<SWidget> ValueWidget = SNullWidget::NullWidget;
        int DesiredWidthScalar = 1;

        if (const FMythicaParameterFloat* FloatParameter = Parameter.Value.TryGet<FMythicaParameterFloat>())
        {
            TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);

            for (int ComponentIndex = 0; ComponentIndex < FloatParameter->Values.Num(); ++ComponentIndex)
            {
                auto Value = [=]()
                {
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                    return Parameters ? Parameters->Parameters[ParamIndex].Value.Get<FMythicaParameterFloat>().Values[ComponentIndex] : 0.0f;
                };

                auto OnValueChanged = [=](float NewValue)
                {
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                    if (Parameters)
                        Parameters->Parameters[ParamIndex].Value.Get<FMythicaParameterFloat>().Values[ComponentIndex] = NewValue;
                };

                HorizontalBox->AddSlot()
                    .Padding(0.0f, 0.0f, 2.0f, 0.0f)
                    [
                        SNew(SNumericEntryBox<float>)
                            .Value_Lambda(Value)
                            .OnValueChanged_Lambda(OnValueChanged)
                            .AllowSpin(true)
                            .MinValue(FloatParameter->MinValue)
                            .MaxValue(FloatParameter->MaxValue)
                            .MinSliderValue(FloatParameter->MinValue)
                            .MaxSliderValue(FloatParameter->MaxValue)
                    ];
            }

            ValueWidget = HorizontalBox;
            DesiredWidthScalar = FloatParameter->Values.Num();
        }
        else if (const FMythicaParameterInt* IntParameter = Parameter.Value.TryGet<FMythicaParameterInt>())
        {
            TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);

            for (int ComponentIndex = 0; ComponentIndex < IntParameter->Values.Num(); ++ComponentIndex)
            {
                auto Value = [=]()
                {
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                    return Parameters ? Parameters->Parameters[ParamIndex].Value.Get<FMythicaParameterInt>().Values[ComponentIndex] : 0;
                };

                auto OnValueChanged = [=](int NewValue)
                {
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                    if (Parameters)
                        Parameters->Parameters[ParamIndex].Value.Get<FMythicaParameterInt>().Values[ComponentIndex] = NewValue;
                };

                HorizontalBox->AddSlot()
                    .Padding(0.0f, 0.0f, 2.0f, 0.0f)
                    [
                        SNew(SNumericEntryBox<int>)
                            .Value_Lambda(Value)
                            .OnValueChanged_Lambda(OnValueChanged)
                            .AllowSpin(true)
                            .MinValue(IntParameter->MinValue)
                            .MaxValue(IntParameter->MaxValue)
                            .MinSliderValue(IntParameter->MinValue)
                            .MaxSliderValue(IntParameter->MaxValue)
                    ];
            }

            ValueWidget = HorizontalBox;
            DesiredWidthScalar = IntParameter->Values.Num();
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

            ValueWidget = SNew(SCheckBox)
                .IsChecked_Lambda(IsChecked)
                .OnCheckStateChanged_Lambda(OnCheckStateChanged);
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

            ValueWidget = SNew(SMultiLineEditableText)
                .Text_Lambda(Text)
                .OnTextCommitted_Lambda(OnTextCommitted)
                .AutoWrapText(true);
            DesiredWidthScalar = 3;
        }
        else
        {
            continue;
        }

        StructBuilder.AddCustomRow(FText::FromString(Parameter.Label))
            .NameContent()
            [
                SNew(STextBlock)
                    .Text(FText::FromString(Parameter.Label))
            ]
            .ValueContent()
            .MinDesiredWidth(DesiredWidthScalar * 128)
            [
                ValueWidget
            ];
    }
}
