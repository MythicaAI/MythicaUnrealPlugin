
#include "MythicaPackageManagerWidget.h"

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#define PACKAGE_MANAGER_WIDGET_ASSET TEXT("/Mythica/UI/WBP_PackageManager.WBP_PackageManager_C")

void SMythicaPackageManagerWidget::Construct(const FArguments& InArgs)
{
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World) 
        return;
    
    UClass* WidgetClass = StaticLoadClass(UUserWidget::StaticClass(), nullptr, PACKAGE_MANAGER_WIDGET_ASSET);
    if (!WidgetClass)
        return;

    UUserWidget* UserWidget = CreateWidget<UUserWidget>(World, WidgetClass);
    if (!UserWidget)
        return;

    ChildSlot
    [
        UserWidget->TakeWidget()
    ];
}
