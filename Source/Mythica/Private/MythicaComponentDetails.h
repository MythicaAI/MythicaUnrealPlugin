#pragma once

#include "IDetailCustomization.h"

class FMythicaComponentDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    TArray<TSharedPtr<FString>> Options;
    TArray<FString> JobDefIds;

    void OnSelectionChanged(const FString& NewValue, TWeakObjectPtr<class UMythicaComponent> ComponentWeak);
    FString GetSelectedOption(TWeakObjectPtr<class UMythicaComponent> ComponentWeak) const;
};
