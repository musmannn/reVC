#include "common.h"

#ifdef MULTIPLAYER

#include "NetGame.h"
#include "MpText.h"
#include "SpawnPoints.h"
#include "Frontend.h"
#include "Messages.h"
#include "PlayerPed.h"
#include "Timer.h"
#include "World.h"

#ifdef LOAD_INI_SETTINGS
void SaveINISettings();
#endif

char CNetGame::ms_playerName[MP_MAX_NAME_LEN + 1] = "Player";
char CNetGame::ms_relayAddress[64] = MP_DEFAULT_RELAY;

static CNetClient sClient;
static bool sWantWorld;		// session ready; next world start is multiplayer
static bool sInGame;		// multiplayer world running
static bool sLoading;		// servicing from the loading screen
static bool sPendingStart;	// start the world at the next safe point
static bool sPendingEnd;	// end-of-session UI at the next safe point
static const char *sStatusKey;
static char sCode[MP_INVITE_CODE_LEN + 1];

struct SRosterEntry
{
	bool used;
	char name[MP_MAX_NAME_LEN + 1];
};
static SRosterEntry sRoster[MP_MAX_PLAYERS];

static void
ClearRoster(void)
{
	memset(sRoster, 0, sizeof(sRoster));
}

static void
ShowMessage(const char *key)
{
	wchar *text = MpGetText(key);
	if(text && !FrontEndMenuManager.m_bGameNotLoaded)
		CMessages::AddMessageJumpQ(text, 5000, 0);
}

// Local teardown after the session is gone (ended, lost or left).
static void
EndLocal(const char *statusKey)
{
	bool wasInGame = sInGame;
	sWantWorld = false;
	sInGame = false;
	sPendingStart = false;
	ClearRoster();
	sStatusKey = statusKey;
	if(wasInGame && statusKey)
		sPendingEnd = true;
}

class CNetGameListener : public INetClientListener
{
public:
	void OnSessionReady(const char *code, uint8 myId, bool isHost) override
	{
		(void)myId;
		(void)isHost;
		strncpy(sCode, code, sizeof(sCode) - 1);
		sCode[sizeof(sCode) - 1] = '\0';
		ClearRoster();
		sRoster[myId].used = true;
		strncpy(sRoster[myId].name, CNetGame::ms_playerName, MP_MAX_NAME_LEN);
		sStatusKey = nil;
		sWantWorld = true;
		sPendingStart = true;
	}
	void OnJoinFailed(eJoinResult result) override
	{
		const char *key = "MP_NOTFOUND";
		switch(result){
		case JOIN_FULL: key = "MP_FULL"; break;
		case JOIN_VERSION_MISMATCH: key = "MP_VERSION"; break;
		case JOIN_BAD_NAME: key = "MP_BADNAME"; break;
		default: break;
		}
		EndLocal(key);
		ShowMessage(key);
	}
	void OnPlayerJoined(uint8 id, const char *name) override
	{
		if(id >= MP_MAX_PLAYERS)
			return;
		sRoster[id].used = true;
		strncpy(sRoster[id].name, name, MP_MAX_NAME_LEN);
		sRoster[id].name[MP_MAX_NAME_LEN] = '\0';
	}
	void OnPlayerLeft(uint8 id) override
	{
		if(id >= MP_MAX_PLAYERS)
			return;
		sRoster[id].used = false;
	}
	void OnSessionEnded(eEndReason reason) override
	{
		EndLocal(reason == END_RELAY_SHUTDOWN ? "MP_ENDED_RELAY" : "MP_ENDED_HOST");
	}
	void OnConnectionLost(void) override
	{
		EndLocal("MP_LOST");
	}
	void OnChat(uint8 from, const char *text) override
	{
		(void)from;
		(void)text;
	}
	void OnGameMessage(uint8 from, uint8 type, CByteReader &body) override
	{
		// Game messages only make sense once the multiplayer world runs.
		if(!sInGame)
			return;
		(void)from;
		(void)type;
		(void)body;
	}
};
static CNetGameListener sListener;

uint32
CNetGame::NetNow(void)
{
	return (uint32)(CTimer::GetCurrentTimeInCycles() / CTimer::GetCyclesPerMillisecond());
}

uint32
CNetGame::SessionNow(void)
{
	return sClient.SessionTime(NetNow());
}

void
CNetGame::Init(void)
{
	sWantWorld = false;
	sInGame = false;
	sPendingStart = false;
	sPendingEnd = false;
	sStatusKey = nil;
	ClearRoster();
}

void
CNetGame::Shutdown(void)
{
	sClient.Leave();
	EndLocal(nil);
}

void
CNetGame::ServiceDuringLoad(void)
{
	// Keeps the connection alive while the world loads. UI reactions wait
	// for the next normal Service().
	sLoading = true;
	sClient.Service(NetNow(), &sListener);
	sLoading = false;
}

void
CNetGame::Service(void)
{
	if(sLoading)
		return;
	sClient.Service(NetNow(), &sListener);

	if(sPendingStart){
		sPendingStart = false;
		// The New Game path: closes the frontend and restarts the world,
		// which loads freeroam_miami.scm because WantsWorld() is true.
		FrontEndMenuManager.DoSettingsBeforeStartingAGame();
	}
	if(sPendingEnd){
		sPendingEnd = false;
		if(sStatusKey)
			ShowMessage(sStatusKey);
		// Back to the frontend, where the Multiplayer page shows the reason.
		if(!FrontEndMenuManager.m_bMenuActive && !FrontEndMenuManager.m_bGameNotLoaded){
			FrontEndMenuManager.m_bStartUpFrontEndRequested = true;
			FrontEndMenuManager.SwitchMenuOnAndOff();
		}
	}
}

void
CNetGame::Process(void)
{
	if(!sInGame)
		return;
}

void
CNetGame::Render2d(void)
{
	if(!sInGame)
		return;
}

bool
CNetGame::IsActive(void)
{
	return sInGame && sClient.IsInSession();
}

bool
CNetGame::IsConnected(void)
{
	return sClient.IsActive();
}

bool
CNetGame::IsHost(void)
{
	return sClient.IsHost();
}

uint8
CNetGame::MyId(void)
{
	return sClient.MyId();
}

CNetClient&
CNetGame::Client(void)
{
	return sClient;
}

// "host" or "host:port" from the Relay setting.
static void
ParseRelay(char *host, int hostSize, uint16 &port)
{
	strncpy(host, CNetGame::ms_relayAddress, hostSize - 1);
	host[hostSize - 1] = '\0';
	port = MP_DEFAULT_RELAY_PORT;
	char *colon = strrchr(host, ':');
	if(colon){
		int p = atoi(colon + 1);
		if(p > 0 && p < 65536)
			port = (uint16)p;
		*colon = '\0';
	}
}

bool
CNetGame::HostSession(void)
{
	if(sClient.IsActive())
		return false;
	char host[64];
	uint16 port;
	ParseRelay(host, sizeof(host), port);
	sStatusKey = nil;
	if(!sClient.Host(host, port, ms_playerName)){
		sStatusKey = "MP_BADRELAY";
		return false;
	}
	ShowMessage("MP_CONNECTING");
	return true;
}

bool
CNetGame::JoinSession(const char *code)
{
	if(sClient.IsActive())
		return false;
	char host[64];
	uint16 port;
	ParseRelay(host, sizeof(host), port);
	sStatusKey = nil;
	if(!sClient.Join(host, port, code, ms_playerName)){
		sStatusKey = "MP_BADRELAY";
		return false;
	}
	ShowMessage("MP_CONNECTING");
	return true;
}

void
CNetGame::LeaveSession(void)
{
	sClient.Leave();
	EndLocal(nil);
}

const char*
CNetGame::InviteCode(void)
{
	return sClient.IsInSession() ? sCode : "";
}

int
CNetGame::NumPlayers(void)
{
	int n = 0;
	for(int i = 0; i < MP_MAX_PLAYERS; i++)
		if(sRoster[i].used)
			n++;
	return n;
}

const char*
CNetGame::PlayerName(uint8 id)
{
	if(id >= MP_MAX_PLAYERS || !sRoster[id].used)
		return nil;
	return sRoster[id].name;
}

const char*
CNetGame::LastStatusKey(void)
{
	return sStatusKey;
}

bool
CNetGame::WantsWorld(void)
{
	return sWantWorld;
}

void
CNetGame::OnWorldStarted(void)
{
	if(!sWantWorld)
		return;
	if(!sClient.IsInSession()){
		// The session died while the world was loading.
		EndLocal(sStatusKey ? sStatusKey : "MP_LOST");
		return;
	}
	sInGame = true;

	CPlayerPed *player = FindPlayerPed();
	if(player){
		CVector pos;
		float heading;
		CSpawnPoints::Pick(sClient.MyId(), pos, heading);
		player->Teleport(pos);
		player->m_fRotationCur = heading;
		player->m_fRotationDest = heading;
		CSpawnPoints::GiveLoadout(player);
	}
}

void
CNetGame::LoadSettings(void)
{
	// Read by LoadINISettings; this sanitizes what it found.
	char clean[MP_MAX_NAME_LEN + 1];
	if(MpSanitizeName(ms_playerName, clean))
		strcpy(ms_playerName, clean);
	else
		strcpy(ms_playerName, "Player");
	if(ms_relayAddress[0] == '\0')
		strcpy(ms_relayAddress, MP_DEFAULT_RELAY);
}

void
CNetGame::SaveSettings(void)
{
#ifdef LOAD_INI_SETTINGS
	SaveINISettings();
#endif
}

#endif
