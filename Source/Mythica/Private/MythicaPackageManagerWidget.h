#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SMythicaPackageManagerWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SMythicaPackageManagerWidget) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
};
