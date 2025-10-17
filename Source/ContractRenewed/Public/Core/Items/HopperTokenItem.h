// @ 2025, Epic MegaJam. All rights reserved.

#pragma once

#include "Core/Items/HopperItem.h"
#include "HopperTokenItem.generated.h"

/** Native base class for tokens/currency, should be blueprinted */
UCLASS()
class CONTRACTRENEWED_API UHopperTokenItem : public UHopperItem
{
	GENERATED_BODY()

public:
	/** Constructor */
	UHopperTokenItem()
	{
		ItemType = UHopperAssetManager::TokenItemType;
		MaxCount = 0; // Infinite
	}
};
