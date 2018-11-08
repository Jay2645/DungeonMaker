

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.h"

#include "GroundScatterItem.h"

#include "AestheticAssetData.generated.h"

/**
 * 
 */
UCLASS()
class DUNGEONMAKER_API UAestheticAssetData : public UDataAsset, public IGameplayTagAssetInterface
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FGameplayTagContainer AssetTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Props")
	FGroundScatterPairing GroundScatter;

public:
	/**
	* Get any owned gameplay tags on the asset
	*
	* @param OutTags	[OUT] Set of tags on the asset
	*/
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override
	{
		TagContainer = AssetTags;
	}
};
