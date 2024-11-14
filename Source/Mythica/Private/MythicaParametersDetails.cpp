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

    HandleWeak = StructPropertyHandle.ToWeakPtr();
    for (int32 ParamIndex = 0; ParamIndex < Parameters->Parameters.Num(); ++ParamIndex)
    {
        const FMythicaParameter& Parameter = Parameters->Parameters[ParamIndex];

        TSharedRef<SWidget> ValueWidget = SNullWidget::NullWidget;
        int DesiredWidthScalar = 1;

        switch (Parameter.Type)
        {
            case EMythicaParameterType::Float:
            {
                TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);

                for (int ComponentIndex = 0; ComponentIndex < Parameter.ValueFloat.Values.Num(); ++ComponentIndex)
                {
                    auto Value = [this, ParamIndex, ComponentIndex]()
                    {
                        FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                        return Parameters ? Parameters->Parameters[ParamIndex].ValueFloat.Values[ComponentIndex] : 0.0f;
                    };

                    auto OnValueChanged = [this, ParamIndex, ComponentIndex](float NewValue)
                    {
                        FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                        if (Parameters)
                        {
                            Parameters->Parameters[ParamIndex].ValueFloat.Values[ComponentIndex] = NewValue;
                            HandleWeak.Pin()->NotifyPostChange(UsingSlider ? EPropertyChangeType::Interactive : EPropertyChangeType::ValueSet);
                        }
                    };

                    auto OnBeginSliderMovement = [this]() 
                    {
                        UsingSlider = true;
                    };

                    auto OnEndSliderMovement = [this](float NewValue)
                    {
                        UsingSlider = false;
                    };

                    HorizontalBox->AddSlot()
                        .Padding(0.0f, 0.0f, 2.0f, 0.0f)
                        [
                            SNew(SNumericEntryBox<float>)
                                .Value_Lambda(Value)
                                .OnValueChanged_Lambda(OnValueChanged)
                                .OnBeginSliderMovement_Lambda(OnBeginSliderMovement)
                                .OnEndSliderMovement_Lambda(OnEndSliderMovement)
                                .AllowSpin(true)
                                .MinValue(Parameter.ValueFloat.MinValue)
                                .MaxValue(Parameter.ValueFloat.MaxValue)
                                .MinSliderValue(Parameter.ValueFloat.MinValue)
                                .MaxSliderValue(Parameter.ValueFloat.MaxValue)
                        ];
                }

                ValueWidget = HorizontalBox;
                DesiredWidthScalar = Parameter.ValueFloat.Values.Num();
                break;
            }
            case EMythicaParameterType::Int:
            {
                TSharedRef<SHorizontalBox> HorizontalBox = SNew(SHorizontalBox);

                for (int ComponentIndex = 0; ComponentIndex < Parameter.ValueInt.Values.Num(); ++ComponentIndex)
                {
                    auto Value = [this, ParamIndex, ComponentIndex]()
                    {
                        FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                        return Parameters ? Parameters->Parameters[ParamIndex].ValueInt.Values[ComponentIndex] : 0;
                    };

                    auto OnValueChanged = [this, ParamIndex, ComponentIndex](int NewValue)
                    {
                        FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                        if (Parameters)
                        {
                            Parameters->Parameters[ParamIndex].ValueInt.Values[ComponentIndex] = NewValue;
                            HandleWeak.Pin()->NotifyPostChange(UsingSlider ? EPropertyChangeType::Interactive : EPropertyChangeType::ValueSet);
                        }
                    };

                    auto OnBeginSliderMovement = [this]()
                    {
                        UsingSlider = true;
                    };

                    auto OnEndSliderMovement = [this](int NewValue)
                    {
                        UsingSlider = false;
                    };

                    HorizontalBox->AddSlot()
                        .Padding(0.0f, 0.0f, 2.0f, 0.0f)
                        [
                            SNew(SNumericEntryBox<int>)
                                .Value_Lambda(Value)
                                .OnValueChanged_Lambda(OnValueChanged)
                                .OnBeginSliderMovement_Lambda(OnBeginSliderMovement)
                                .OnEndSliderMovement_Lambda(OnEndSliderMovement)
                                .AllowSpin(true)
                                .MinValue(Parameter.ValueInt.MinValue)
                                .MaxValue(Parameter.ValueInt.MaxValue)
                                .MinSliderValue(Parameter.ValueInt.MinValue)
                                .MaxSliderValue(Parameter.ValueInt.MaxValue)
                        ];
                }

                ValueWidget = HorizontalBox;
                DesiredWidthScalar = Parameter.ValueInt.Values.Num();
                break;
            }
            case EMythicaParameterType::Bool:
            {
                auto IsChecked = [this, ParamIndex]()
                {
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                    bool Value = Parameters ? Parameters->Parameters[ParamIndex].ValueBool.Value : false;
                    return Value ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
                };

                auto OnCheckStateChanged = [this, ParamIndex](ECheckBoxState NewState)
                {
                    if (NewState == ECheckBoxState::Undetermined)
                        return;

                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                    if (Parameters)
                    {
                        Parameters->Parameters[ParamIndex].ValueBool.Value = (NewState == ECheckBoxState::Checked);
                        HandleWeak.Pin()->NotifyPostChange(EPropertyChangeType::ValueSet);
                    }
                };

                ValueWidget = SNew(SCheckBox)
                    .IsChecked_Lambda(IsChecked)
                    .OnCheckStateChanged_Lambda(OnCheckStateChanged);
                break;
            }
            case EMythicaParameterType::String:
            {
                auto Text = [this, ParamIndex]()
                {
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                    return Parameters ? FText::FromString(Parameters->Parameters[ParamIndex].ValueString.Value) : FText();
                };

                auto OnTextCommitted = [this, ParamIndex](const FText& InText, ETextCommit::Type InCommitType)
                {
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                    if (Parameters)
                    {
                        Parameters->Parameters[ParamIndex].ValueString.Value = InText.ToString();
                        HandleWeak.Pin()->NotifyPostChange(EPropertyChangeType::ValueSet);
                    }
                };

                ValueWidget = SNew(SMultiLineEditableText)
                    .Text_Lambda(Text)
                    .OnTextCommitted_Lambda(OnTextCommitted)
                    .AutoWrapText(true);
                DesiredWidthScalar = 3;
                break;
            }
            case EMythicaParameterType::Enum:
            {
                // Generate combo box options array
                TSharedPtr<TEnumOptions> Options = MakeShared<TEnumOptions>();
                EnumOptionSets.Add(Options);

                const FMythicaParameterEnum& EnumParam = Parameters->Parameters[ParamIndex].ValueEnum;
                for (const FMythicaParameterEnumValue& EnumValue : EnumParam.Values)
                {
                    Options->Add(MakeShared<FMythicaParameterEnumValue>(EnumValue));
                }

                // Find the item with the current value name
                TSharedPtr<FMythicaParameterEnumValue> CurrentItem;
                for (const TSharedPtr<FMythicaParameterEnumValue>& Option : *Options)
                {
                    if (Option->Name == EnumParam.Value)
                    {
                        CurrentItem = Option;
                        break;
                    }
                }

                auto OnSelectionChanged = [this, ParamIndex](TSharedPtr<FMythicaParameterEnumValue> NewSelection, ESelectInfo::Type SelectInfo)
                {
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                    if (Parameters && NewSelection.IsValid())
                    {
                        Parameters->Parameters[ParamIndex].ValueEnum.Value = NewSelection->Name;
                        HandleWeak.Pin()->NotifyPostChange(EPropertyChangeType::ValueSet);
                    }
                };

                auto OnGenerateWidget = [](TSharedPtr<FMythicaParameterEnumValue> InItem)
                {
                    return SNew(STextBlock)
                        .Text(FText::FromString(InItem->Label));
                };

                auto GetCurrentText = [this, ParamIndex]()
                {
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                    if (Parameters)
                    {
                        const FMythicaParameterEnum& EnumParam = Parameters->Parameters[ParamIndex].ValueEnum;
                        for (const FMythicaParameterEnumValue& EnumValue : EnumParam.Values)
                        {
                            if (EnumValue.Name == EnumParam.Value)
                            {
                                return FText::FromString(EnumValue.Label);
                            }
                        }
                    }
                    return FText::FromString(TEXT(""));
                };

                ValueWidget = SNew(SComboBox<TSharedPtr<FMythicaParameterEnumValue>>)
                    .OptionsSource(Options.Get())
                    .OnSelectionChanged_Lambda(OnSelectionChanged)
                    .OnGenerateWidget_Lambda(OnGenerateWidget)
                    .InitiallySelectedItem(CurrentItem)
                    [
                        SNew(STextBlock)
                            .Text_Lambda(GetCurrentText)
                    ];
                break;
            }
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
