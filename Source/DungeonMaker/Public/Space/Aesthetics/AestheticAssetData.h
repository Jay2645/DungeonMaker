

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "GroundScatterItem.h"

#include "AestheticAssetData.generated.h"

/**
 * 
 */
UCLASS()
class DUNGEONMAKER_API UAestheticAssetData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FGameplayTagContainer AssetTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Props")
	FGroundScatterPairing GroundScatter;
};
