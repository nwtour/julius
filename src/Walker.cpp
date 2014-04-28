#include "Walker.h"

#include "Trader.h"

#include "Data/Building.h"
#include "Data/CityInfo.h"
#include "Data/Empire.h"
#include "Data/Grid.h"
#include "Data/Random.h"
#include "Data/Settings.h"
#include "Data/Walker.h"

#include <string.h>

void Walker_clearList()
{
	for (int i = 0; i < MAX_WALKERS; i++) {
		memset(&Data_Walkers[i], 0, 128);
	}
	Data_Walker_Extra.highestWalkerIdEver = 0;
}

int Walker_create(int walkerType, int x, int y, char direction)
{
	int id = 0;
	for (int i = 1; i < MAX_WALKERS; i++) {
		if (!Data_Walkers[i].state) {
			id = i;
			break;
		}
	}
	if (!id) {
		return 0;
	}
	struct Data_Walker *w = &Data_Walkers[id];
	w->state = 1;
	w->ciid = 1;
	w->type = walkerType;
	w->__unknown_0c = 0;
	w->isFriendly = 1;
	w->createdSequence = Data_Walker_Extra.createdSequence++;
	w->direction = direction;
	w->sourceX = w->destinationX = w->previousTileX = w->x = x;
	w->sourceY = w->destinationY = w->previousTileY = w->y = y;
	w->gridOffset = GridOffset(x, y);
	w->tilePositionX = 15 * x;
	w->tilePositionY = 15 * y;
	w->progressOnTile = 15;
	w->phraseSequenceCity = w->phraseSequenceExact = Data_Random.random1_7bit & 3;
	WalkerName_set(id);
	Walker_addToTileList(id);
	if (walkerType == Walker_TradeCaravan || walkerType == Walker_TradeShip) {
		Trader_create(id);
	}
	if (id > Data_Walker_Extra.highestWalkerIdEver) {
		Data_Walker_Extra.highestWalkerIdEver = id;
	}
	return id;
}

void Walker_delete(int walkerId)
{
	struct Data_Walker *w = &Data_Walkers[walkerId];
	if (w->type == Walker_EnemyCaesarLegionary) {
		Data_CityInfo.caesarInvasionSoldiersDied++;
	}
	switch (w->type) {
		case Walker_LaborSeeker:
		case Walker_MarketBuyer:
			if (w->buildingId) {
				Data_Buildings[w->buildingId].walkerId2 = 0;
			}
			break;
		case Walker_Ballista:
			Data_Buildings[w->buildingId].walkerId4 = 0;
			break;
		case Walker_Dockman:
			for (int i = 0; i < 3; i++) {
				if (Data_Buildings[w->buildingId].data.other.dockWalkerIds[i] == walkerId) {
					Data_Buildings[w->buildingId].data.other.dockWalkerIds[i] = 0;
				}
			}
			break;
		case Walker_Explosion:
		case Walker_FortStandard:
		case Walker_Arrow:
		case Walker_Javelin:
		case Walker_Bolt:
		case Walker_Spear:
		case Walker_FishGulls:
		case Walker_Sheep:
		case Walker_Wolf:
		case Walker_Zebra:
		case Walker_DeliveryBoy:
		case Walker_Patrician:
			// nothing to do here
			break;
		default:
			if (w->buildingId) {
				Data_Buildings[w->buildingId].walkerId = 0;
			}
			break;
	}
	if (w->empireCityId) {
		for (int i = 0; i < 3; i++) {
			if (Data_Empire_Cities[w->empireCityId].traderWalkerIds[i] == walkerId) {
				Data_Empire_Cities[w->empireCityId].traderWalkerIds[i] = 0;
			}
		}
	}
	if (w->immigrantBuildingId) {
		Data_Buildings[w->buildingId].immigrantWalkerId = 0;
	}
	WalkerRoute_remove(walkerId);
	Walker_removeFromTileList(walkerId);
	memset(w, 0, 128);
}

void Walker_addToTileList(int walkerId)
{
	if (Data_Walkers[walkerId].gridOffset < 0) {
		return;
	}
	struct Data_Walker *w = &Data_Walkers[walkerId];
	w->numPreviousWalkersOnSameTile = 0;

	int next = Data_Grid_walkerIds[w->gridOffset];
	if (next) {
		w->numPreviousWalkersOnSameTile++;
		while (Data_Walkers[next].nextWalkerIdOnSameTile) {
			next = Data_Walkers[next].nextWalkerIdOnSameTile;
			w->numPreviousWalkersOnSameTile++;
		}
		if (w->numPreviousWalkersOnSameTile > 20) {
			w->numPreviousWalkersOnSameTile = 20;
		}
		Data_Walkers[next].nextWalkerIdOnSameTile = walkerId;
	} else {
		Data_Grid_walkerIds[w->gridOffset] = walkerId;
	}
}

void Walker_updatePositionInTileList(int walkerId)
{
	struct Data_Walker *w = &Data_Walkers[walkerId];
	w->numPreviousWalkersOnSameTile = 0;
	
	int next = Data_Grid_walkerIds[w->gridOffset];
	while (next) {
		if (next == walkerId) {
			return;
		}
		w->numPreviousWalkersOnSameTile++;
		next = Data_Walkers[next].nextWalkerIdOnSameTile;
	}
	if (w->numPreviousWalkersOnSameTile > 20) {
		w->numPreviousWalkersOnSameTile = 20;
	}
}

void Walker_removeFromTileList(int walkerId)
{
	if (Data_Walkers[walkerId].gridOffset < 0) {
		return;
	}
	struct Data_Walker *w = &Data_Walkers[walkerId];

	int cur = Data_Grid_walkerIds[w->gridOffset];
	if (cur) {
		if (cur == walkerId) {
			Data_Grid_walkerIds[w->gridOffset] = w->nextWalkerIdOnSameTile;
			w->nextWalkerIdOnSameTile = 0;
		} else {
			while (cur && Data_Walkers[cur].nextWalkerIdOnSameTile != walkerId) {
				cur = Data_Walkers[cur].nextWalkerIdOnSameTile;
			}
			Data_Walkers[cur].nextWalkerIdOnSameTile = w->nextWalkerIdOnSameTile;
			w->nextWalkerIdOnSameTile = 0;
		}
	}
}

int Walker_TradeCaravan_isBuying(int walkerId, int buildingId, int empireCityId)
{
	// TODO
	return 0;
}

int Walker_TradeCaravan_isSelling(int walkerId, int buildingId, int empireCityId)
{
	// TODO
	return 0;
}

int Walker_TradeShip_isBuyingOrSelling(int walkerId)
{
	// TODO
	return 0;
}