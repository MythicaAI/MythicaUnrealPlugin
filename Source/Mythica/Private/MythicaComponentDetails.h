#pragma once

#include "IDetailCustomization.h"

struct FMythicaToolOptionData
{
    FString JobDefId;
    FString Name;
    FString Owner;
    FString AssetName;
    FString Version;
};

class FMythicaComponentDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    TArray<TSharedPtr<FString>> Options;
    TArray<FMythicaToolOptionData> OptionData;

    void PopulateToolOptions();
    void SelectTool(const FString& JobDefId, TWeakObjectPtr<class UMythicaComponent> ComponentWeak);
    FString GetSelectedOption(TWeakObjectPtr<class UMythicaComponent> ComponentWeak) const;
};
