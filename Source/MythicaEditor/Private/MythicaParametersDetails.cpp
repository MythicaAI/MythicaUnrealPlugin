#include "MythicaParametersDetails.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "UI/Slate/MythicaCurveEditor.h"
#include "MythicaTypes.h"
#include "Styling/AppStyle.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "Widgets/Input/SNumericEntryBox.h"

#include <functional>

#define LOCTEXT_NAMESPACE "Mythica"

static FMythicaParameters* GetParametersFromHandle(IPropertyHandle& Handle, UObject** OutObject = nullptr)
{
    TArray<UObject*> Objects;
    Handle.GetOuterObjects(Objects);
    if (Objects.Num() != 1)
    {
        return nullptr;
    }

    if (OutObject)
    {
        *OutObject = Objects[0];
    }

    return (FMythicaParameters*)Handle.GetValueBaseAddress((uint8*)Objects[0]);
}

static FMythicaParameters* GetParametersFromHandleWeak(TWeakPtr<IPropertyHandle> HandleWeak, UObject** OutObject = nullptr)
{
    if (!HandleWeak.IsValid())
        return nullptr;

    IPropertyHandle* Handle = HandleWeak.Pin().Get();
    if (!Handle)
        return nullptr;

    return GetParametersFromHandle(*Handle, OutObject);
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
        if (Mythica::IsSystemParameter(Parameter.Name))
        {
            continue;
        }

        TSharedRef<SWidget> ValueWidget = SNullWidget::NullWidget;
        int DesiredWidthScalar = 1;
        std::function<EVisibility()> ResetToDefaultVisible;
        std::function<FReply()> OnResetToDefault;

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
                        UObject* Object = nullptr;
                        FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak, &Object);
                        if (Parameters)
                        {
                            float& Value = Parameters->Parameters[ParamIndex].ValueFloat.Values[ComponentIndex];
                            if (Value != NewValue)
                            {
                                Object->Modify();
                                Value = NewValue;
                                HandleWeak.Pin()->NotifyPostChange(UsingSlider ? EPropertyChangeType::Interactive : EPropertyChangeType::ValueSet);
                            }
                        }
                    };

                    auto OnValueCommitted = [this, ParamIndex, ComponentIndex](float NewValue, ETextCommit::Type)
                    {
                        UObject* Object = nullptr;
                        FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak, &Object);
                        if (Parameters)
                        {
                            float& Value = Parameters->Parameters[ParamIndex].ValueFloat.Values[ComponentIndex];
                            if (Value != NewValue)
                            {
                                const FScopedTransaction Transaction(LOCTEXT("MythicaChangeParameter", "Parameter Value Changed"));
                                Object->Modify();
                                Value = NewValue;
                                HandleWeak.Pin()->NotifyPostChange(UsingSlider ? EPropertyChangeType::Interactive : EPropertyChangeType::ValueSet);
                            }
                        }
                    };

                    auto OnBeginSliderMovement = [this]() 
                    {
                        UsingSlider = true;
                        GEditor->BeginTransaction(TEXT("Mythica"), LOCTEXT("MythicaChangeParameter", "Parameter Value Changed"), nullptr);
                    };

                    auto OnEndSliderMovement = [this](float NewValue)
                    {
                        UsingSlider = false;
                        GEditor->EndTransaction();
                    };

                    HorizontalBox->AddSlot()
                        .Padding(0.0f, 0.0f, 2.0f, 0.0f)
                        [
                            SNew(SNumericEntryBox<float>)
                                .Value_Lambda(Value)
                                .OnValueChanged_Lambda(OnValueChanged)
                                .OnValueCommitted_Lambda(OnValueCommitted)
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

                ResetToDefaultVisible = [this, ParamIndex]()
                {
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                    if (Parameters)
                    {
                        const FMythicaParameterFloat& FloatParam = Parameters->Parameters[ParamIndex].ValueFloat;
                        for (int i = 0; i < FloatParam.Values.Num(); ++i)
                        {
                            if (FloatParam.Values[i] != FloatParam.DefaultValues[i])
                            {
                                return EVisibility::Visible;
                            }
                        }
                    }

                    return EVisibility::Collapsed;
                };

                OnResetToDefault = [this, ParamIndex]()
                {
                    UObject* Object = nullptr;
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak, &Object);
                    if (Parameters)
                    {
                        const FScopedTransaction Transaction(LOCTEXT("MythicaChangeParameter", "Parameter Value Reset"));
                        Object->Modify();

                        FMythicaParameterFloat& FloatParam = Parameters->Parameters[ParamIndex].ValueFloat;
                        for (int i = 0; i < FloatParam.Values.Num(); ++i)
                        {
                            FloatParam.Values[i] = FloatParam.DefaultValues[i];
                        }
                        HandleWeak.Pin()->NotifyPostChange(EPropertyChangeType::ValueSet);
                    }

                    return FReply::Handled();
                };
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
                        UObject* Object = nullptr;
                        FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak, &Object);
                        if (Parameters)
                        {
                            int& Value = Parameters->Parameters[ParamIndex].ValueInt.Values[ComponentIndex];
                            if (Value != NewValue)
                            {
                                Object->Modify();
                                Value = NewValue;
                                HandleWeak.Pin()->NotifyPostChange(UsingSlider ? EPropertyChangeType::Interactive : EPropertyChangeType::ValueSet);
                            }
                        }
                    };

                    auto OnValueCommitted = [this, ParamIndex, ComponentIndex](int NewValue, ETextCommit::Type)
                    {
                        UObject* Object = nullptr;
                        FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak, &Object);
                        if (Parameters)
                        {
                            int& Value = Parameters->Parameters[ParamIndex].ValueInt.Values[ComponentIndex];
                            if (Value != NewValue)
                            {
                                const FScopedTransaction Transaction(LOCTEXT("MythicaChangeParameter", "Parameter Value Changed"));
                                Object->Modify();
                                Value = NewValue;
                                HandleWeak.Pin()->NotifyPostChange(UsingSlider ? EPropertyChangeType::Interactive : EPropertyChangeType::ValueSet);
                            }
                        }
                    };

                    auto OnBeginSliderMovement = [this]()
                    {
                        UsingSlider = true;
                        GEditor->BeginTransaction(TEXT("Mythica"), LOCTEXT("MythicaChangeParameter", "Parameter Value Changed"), nullptr);
                    };

                    auto OnEndSliderMovement = [this](int NewValue)
                    {
                        UsingSlider = false;
                        GEditor->EndTransaction();
                    };

                    HorizontalBox->AddSlot()
                        .Padding(0.0f, 0.0f, 2.0f, 0.0f)
                        [
                            SNew(SNumericEntryBox<int>)
                                .Value_Lambda(Value)
                                .OnValueChanged_Lambda(OnValueChanged)
                                .OnValueCommitted_Lambda(OnValueCommitted)
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

                ResetToDefaultVisible = [this, ParamIndex]()
                {
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                    if (Parameters)
                    {
                        const FMythicaParameterInt& IntParam = Parameters->Parameters[ParamIndex].ValueInt;
                        for (int i = 0; i < IntParam.Values.Num(); ++i)
                        {
                            if (IntParam.Values[i] != IntParam.DefaultValues[i])
                            {
                                return EVisibility::Visible;
                            }
                        }
                    }

                    return EVisibility::Collapsed;
                };

                OnResetToDefault = [this, ParamIndex]()
                {
                    UObject* Object = nullptr;
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak, &Object);
                    if (Parameters)
                    {
                        const FScopedTransaction Transaction(LOCTEXT("MythicaChangeParameter", "Parameter Value Reset"));
                        Object->Modify();

                        FMythicaParameterInt& IntParam = Parameters->Parameters[ParamIndex].ValueInt;
                        for (int i = 0; i < IntParam.Values.Num(); ++i)
                        {
                            IntParam.Values[i] = IntParam.DefaultValues[i];
                        }
                        HandleWeak.Pin()->NotifyPostChange(EPropertyChangeType::ValueSet);
                    }

                    return FReply::Handled();
                };
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

                        UObject* Object = nullptr;
                        FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak, &Object);
                        if (Parameters)
                        {
                            const FScopedTransaction Transaction(LOCTEXT("MythicaChangeParameter", "Parameter Value Changed"));
                            Object->Modify();
                            Parameters->Parameters[ParamIndex].ValueBool.Value = (NewState == ECheckBoxState::Checked);
                            HandleWeak.Pin()->NotifyPostChange(EPropertyChangeType::ValueSet);
                        }
                    };

                ValueWidget = SNew(SCheckBox)
                    .IsChecked_Lambda(IsChecked)
                    .OnCheckStateChanged_Lambda(OnCheckStateChanged);

                ResetToDefaultVisible = [this, ParamIndex]()
                    {
                        FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                        if (Parameters)
                        {
                            const FMythicaParameterBool& BoolParam = Parameters->Parameters[ParamIndex].ValueBool;
                            if (BoolParam.Value != BoolParam.DefaultValue)
                            {
                                return EVisibility::Visible;
                            }
                        }

                        return EVisibility::Collapsed;
                    };

                OnResetToDefault = [this, ParamIndex]()
                    {
                        UObject* Object = nullptr;
                        FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak, &Object);
                        if (Parameters)
                        {
                            const FScopedTransaction Transaction(LOCTEXT("MythicaChangeParameter", "Parameter Value Reset"));
                            Object->Modify();

                            FMythicaParameterBool& BoolParam = Parameters->Parameters[ParamIndex].ValueBool;
                            BoolParam.Value = BoolParam.DefaultValue;
                            HandleWeak.Pin()->NotifyPostChange(EPropertyChangeType::ValueSet);
                        }

                        return FReply::Handled();
                    };
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
                    UObject* Object = nullptr;
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak, &Object);
                    if (Parameters)
                    {
                        const FScopedTransaction Transaction(LOCTEXT("MythicaChangeParameter", "Parameter Value Changed"));
                        Object->Modify();
                        Parameters->Parameters[ParamIndex].ValueString.Value = InText.ToString();
                        HandleWeak.Pin()->NotifyPostChange(EPropertyChangeType::ValueSet);
                    }
                };

                ValueWidget = SNew(SMultiLineEditableText)
                    .Text_Lambda(Text)
                    .OnTextCommitted_Lambda(OnTextCommitted)
                    .AutoWrapText(true);
                DesiredWidthScalar = 3;

                ResetToDefaultVisible = [this, ParamIndex]()
                {
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                    if (Parameters)
                    {
                        const FMythicaParameterString& StringParam = Parameters->Parameters[ParamIndex].ValueString;
                        if (StringParam.Value != StringParam.DefaultValue)
                        {
                            return EVisibility::Visible;
                        }
                    }

                    return EVisibility::Collapsed;
                };

                OnResetToDefault = [this, ParamIndex]()
                {
                    UObject* Object = nullptr;
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak, &Object);
                    if (Parameters)
                    {
                        const FScopedTransaction Transaction(LOCTEXT("MythicaChangeParameter", "Parameter Value Reset"));
                        Object->Modify();

                        FMythicaParameterString& StringParam = Parameters->Parameters[ParamIndex].ValueString;
                        StringParam.Value = StringParam.DefaultValue;
                        HandleWeak.Pin()->NotifyPostChange(EPropertyChangeType::ValueSet);
                    }

                    return FReply::Handled();
                };
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
                    UObject* Object = nullptr;
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak, &Object);
                    if (Parameters && NewSelection.IsValid())
                    {
                        const FScopedTransaction Transaction(LOCTEXT("MythicaChangeParameter", "Parameter Value Changed"));
                        Object->Modify();
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

                ResetToDefaultVisible = [this, ParamIndex]()
                {
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                    if (Parameters)
                    {
                        const FMythicaParameterEnum& EnumParam = Parameters->Parameters[ParamIndex].ValueEnum;
                        if (EnumParam.Value != EnumParam.DefaultValue)
                        {
                            return EVisibility::Visible;
                        }
                    }

                    return EVisibility::Collapsed;
                };

                OnResetToDefault = [this, ParamIndex]()
                {
                    UObject* Object = nullptr;
                    FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak, &Object);
                    if (Parameters)
                    {
                        const FScopedTransaction Transaction(LOCTEXT("MythicaChangeParameter", "Parameter Value Reset"));
                        Object->Modify();

                        FMythicaParameterEnum& EnumParam = Parameters->Parameters[ParamIndex].ValueEnum;
                        EnumParam.Value = EnumParam.DefaultValue;
                        HandleWeak.Pin()->NotifyPostChange(EPropertyChangeType::ValueSet);
                    }

                    return FReply::Handled();
                };
                break;
            }
            case EMythicaParameterType::File:
            {
                const TSharedPtr<IPropertyHandleArray> ParametersArrayHandle = StructPropertyHandle->GetChildHandle(TEXT("Parameters"))->AsArray();
                const TSharedPtr<IPropertyHandle> ParametersHandle = ParametersArrayHandle->GetElement(ParamIndex);
                const TSharedPtr<IPropertyHandle> FileValueHandle = ParametersHandle->GetChildHandle(TEXT("ValueFile"));

                StructBuilder.AddProperty(FileValueHandle.ToSharedRef())
                    .DisplayName(FText::FromString(Parameter.Label));

                continue;
            }
            case EMythicaParameterType::Curve:
            {
                ValueWidget = SNew(SMythicaCurveEditor);
                DesiredWidthScalar = 3;

                //ResetToDefaultVisible = [this, ParamIndex]()
                //    {
                //        FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak);
                //        if (Parameters)
                //        {
                //            const FMythicaParameterCurve& CurveParam = Parameters->Parameters[ParamIndex].ValueCurve;
                //            if (CurveParam.Value != CurveParam.DefaultValue)
                //            {
                //                return EVisibility::Visible;
                //            }
                //        }

                //        return EVisibility::Collapsed;
                //    };

                //OnResetToDefault = [this, ParamIndex]()
                //    {
                //        UObject* Object = nullptr;
                //        FMythicaParameters* Parameters = GetParametersFromHandleWeak(HandleWeak, &Object);
                //        if (Parameters)
                //        {
                //            const FScopedTransaction Transaction(LOCTEXT("MythicaChangeParameter", "Parameter Value Reset"));
                //            Object->Modify();

                //            FMythicaParameterBool& BoolParam = Parameters->Parameters[ParamIndex].ValueBool;
                //            BoolParam.Value = BoolParam.DefaultValue;
                //            HandleWeak.Pin()->NotifyPostChange(EPropertyChangeType::ValueSet);
                //        }

                //        return FReply::Handled();
                //    };
                continue;
            }
        }

        StructBuilder.AddCustomRow(FText::FromString(Parameter.Label))
            .NameContent()
            [
                SNew(STextBlock)
                    .Text(FText::FromString(Parameter.Label))
                    .Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
            ]
            .ValueContent()
            .MinDesiredWidth(DesiredWidthScalar * 128)
            [
                ValueWidget
            ].
            ExtensionContent()
            [
                SNew(SButton)
                    .ButtonStyle(FAppStyle::Get(), "NoBorder")
                    .ContentPadding(0)
                    .Visibility_Lambda(ResetToDefaultVisible)
                    .OnClicked_Lambda(OnResetToDefault)
                    [
                        SNew(SImage)
                            .Image(FAppStyle::Get().GetBrush("PropertyWindow.DiffersFromDefault"))
                    ]
            ];
    }
}
