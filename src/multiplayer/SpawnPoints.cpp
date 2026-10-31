#include "common.h"

#ifdef MULTIPLAYER

#include "SpawnPoints.h"
#include "PlayerPed.h"
#include "Streaming.h"
#include "WeaponInfo.h"
#include "General.h"

struct SSpawnPoint
{
	float x, y, z;
	float headingDeg;
};

// Around the Ocean View Hotel, Ocean Beach.
// TODO verify in-game: placeholder coordinates until the game is installed.
static const SSpawnPoint sSpawnPoints[] = {
	{ 228.0f, -1276.0f, 12.0f, 90.0f },
	{ 228.0f, -1280.0f, 12.0f, 90.0f },
	{ 224.0f, -1276.0f, 12.0f, 90.0f },
	{ 224.0f, -1280.0f, 12.0f, 90.0f },
	{ 232.0f, -1276.0f, 12.0f, 90.0f },
	{ 232.0f, -1280.0f, 12.0f, 90.0f },
	{ 228.0f, -1284.0f, 12.0f, 90.0f },
	{ 224.0f, -1284.0f, 12.0f, 90.0f },
};

void
CSpawnPoints::Pick(uint8 playerId, CVector &pos, float &heading)
{
	const SSpawnPoint &p = sSpawnPoints[playerId % ARRAY_SIZE(sSpawnPoints)];
	pos = CVector(p.x, p.y, p.z);
	heading = DEGTORAD(p.headingDeg);
}

static void
GiveWithModel(CPlayerPed *player, eWeaponType weapon, uint32 ammo)
{
	CWeaponInfo *info = CWeaponInfo::GetWeaponInfo(weapon);
	if(info->m_nModelId > 0)
		CStreaming::RequestModel(info->m_nModelId, STREAMFLAGS_DONT_REMOVE);
	if(info->m_nModel2Id > 0)
		CStreaming::RequestModel(info->m_nModel2Id, STREAMFLAGS_DONT_REMOVE);
	CStreaming::LoadAllRequestedModels(false);
	player->GiveWeapon(weapon, ammo, true);
}

void
CSpawnPoints::GiveLoadout(CPlayerPed *player)
{
	if(player == nil)
		return;
	GiveWithModel(player, WEAPONTYPE_BASEBALLBAT, 1);
	GiveWithModel(player, WEAPONTYPE_COLT45, 68);
	player->SetCurrentWeapon(WEAPONTYPE_COLT45);
	player->m_fHealth = 100.0f;
	player->m_fArmour = 0.0f;
}

#endif
