#include "common.h"

#ifdef MULTIPLAYER

#include "NetMatch.h"
#include "NetGame.h"
#include "NetChat.h"
#include "NetRace.h"
#include "NetVehicles.h"
#include "FreeroamMenu.h"
#include "RemotePlayer.h"
#include "SpawnPoints.h"
#include "MpText.h"
#include "MatchHost.h"
#include "DMAudio.h"
#include "Font.h"
#include "Pad.h"
#include "PlayerInfo.h"
#include "PlayerPed.h"
#include "Sprite2d.h"
#include "Streaming.h"
#include "World.h"

static const uint32 RESULTS_SHOW_MS = 10000;

class CGameMatchSink : public IMatchHostSink
{
	static void Send(uint8 route, uint8 target, bool reliable, const CByteWriter &w)
	{
		CNetGame::Client().SendGame(route, target, reliable, w);
	}
public:
	void SendInvite(const SMatchInvite &m) override { CByteWriter w; m.Write(w); Send(MP_ROUTE_ALL, 0, true, w); }
	void SendStart(const SMatchStart &m) override { CByteWriter w; m.Write(w); Send(MP_ROUTE_ALL, 0, true, w); }
	void SendScores(const SMatchScores &m) override { CByteWriter w; m.Write(w); Send(MP_ROUTE_ALL, 0, false, w); }
	void SendEnd(const SMatchScores &m) override { CByteWriter w; m.Write(w); Send(MP_ROUTE_ALL, 0, true, w); }
	void SendCancel(const SMatchCancel &m, uint8 target) override
	{
		CByteWriter w;
		m.Write(w);
		if(target == MP_PLAYER_NONE)
			Send(MP_ROUTE_ALL, 0, true, w);
		else
			Send(MP_ROUTE_PLAYER, target, true, w);
	}
};
static CMatchHost sHost;
static CGameMatchSink sSink;

struct SMatchInviteState
{
	bool open;
	bool accepted;
	SMatchInvite invite;
};
static SMatchInviteState sInvite;

enum eMatchPhase { PHASE_NONE, PHASE_COUNTDOWN, PHASE_PLAYING };
struct SLocalMatch
{
	eMatchPhase phase;
	SMatchStart start;
	uint8 myTeam;
	int countdownShown;
};
static SLocalMatch sMatch;
static SMatchScores sScores;		// latest, for the HUD and the scoreboard
static SMatchScores sResults;
static uint8 sResultsMode;
static uint32 sResultsUntil;		// NetNow
static char sNames[MP_MAX_PLAYERS][MP_MAX_NAME_LEN + 1];	// cached at START
static bool sMenuRegistered;
static uint16 sLastFinishedMatch;	// ended or cancelled: its late SCORES are dropped
static bool sHaveFinished;

static const char*
NameOf(uint8 id)
{
	// The name at the start of the match first: ids get reused.
	if(id < MP_MAX_PLAYERS && sNames[id][0])
		return sNames[id];
	const char *n = CNetGame::PlayerName(id);
	return n ? n : "?";
}

static const char*
ModeName(uint8 mode)
{
	return mode == MATCH_TDM ? "Team Deathmatch" : "Deathmatch";
}

static CRGBA
TeamRGBA(uint8 team)
{
	return team == MP_TEAM_BLUE ? CRGBA(80, 140, 255, 255) : CRGBA(255, 70, 70, 255);
}

// ---- Menu

static int
ModeCount(void)
{
	return 2;
}

static const char*
ModeLabel(int i)
{
	return i == 1 ? "Team Deathmatch" : "Deathmatch";
}

void
CNetMatch::RequestMatch(int mode)
{
	SMatchRequest msg;
	msg.mode = mode == 1 ? (uint8)MATCH_TDM : (uint8)MATCH_DM;
	CByteWriter w;
	msg.Write(w);
	CNetGame::Client().SendGame(MP_ROUTE_HOST, 0, true, w);
}

void
CNetMatch::Init(void)
{
	if(!sMenuRegistered){
		CFreeroamMenu::AddList("Start match >", ModeCount, ModeLabel, RequestMatch);
		sMenuRegistered = true;
	}
}

// ---- Local player

bool
CNetMatch::IsPlaying(void)
{
	return sMatch.phase != PHASE_NONE;
}

bool
CNetMatch::HostBusy(void)
{
	return CNetGame::IsHost() && sHost.IsBusy();
}

bool
CNetMatch::IsTeammate(uint8 id)
{
	if(sMatch.phase == PHASE_NONE || sMatch.start.mode != MATCH_TDM)
		return false;
	for(int i = 0; i < sMatch.start.n; i++)
		if(sMatch.start.ids[i] == id)
			return sMatch.start.teams[i] == sMatch.myTeam;
	return false;
}

bool
CNetMatch::TeamColour(uint8 id, CRGBA &out)
{
	if(sMatch.phase == PHASE_NONE || sMatch.start.mode != MATCH_TDM)
		return false;
	for(int i = 0; i < sMatch.start.n; i++)
		if(sMatch.start.ids[i] == id){
			out = TeamRGBA(sMatch.start.teams[i]);
			return true;
		}
	return false;
}

void
CNetMatch::GiveMatchLoadout(CPlayerPed *player)
{
	if(player == nil)
		return;
	player->ClearWeapons();	// the same loadout for everyone, every life
	CSpawnPoints::GiveWeaponWithModel(player, WEAPONTYPE_COLT45, 120);
	CSpawnPoints::GiveWeaponWithModel(player, WEAPONTYPE_UZI, 300);
	CSpawnPoints::GiveWeaponWithModel(player, WEAPONTYPE_SHOTGUN, 40);
	player->SetCurrentWeapon(WEAPONTYPE_UZI);
	player->m_fHealth = CWorld::Players[CWorld::PlayerInFocus].m_nMaxHealth;
	player->m_fArmour = 100.0f;
}

static void
Freeze(bool on)
{
	CPad *pad = CPad::GetPad(0);
	if(on)
		pad->SetDisablePlayerControls(CNetMatch::PLAYERCONTROL_MP_MATCH);
	else
		pad->SetEnablePlayerControls(CNetMatch::PLAYERCONTROL_MP_MATCH);
}

static void
SpawnForMatch(void)
{
	CPlayerPed *player = FindPlayerPed();
	if(player == nil || player->DyingOrDead())
		return;	// the respawn puts us in, with the match loadout
	if(player->bInVehicle)
		CNetVehicles::WarpLocalPlayerOut();
	CVector pos;
	float heading;
	CSpawnPoints::Pick(CNetGame::MyId(), pos, heading);
	CStreaming::LoadSceneCollision(pos);
	player->Teleport(pos);
	player->m_fRotationCur = player->m_fRotationDest = heading;
	player->SetHeading(heading);
	CNetMatch::GiveMatchLoadout(player);
}

static void
LeaveMatch(void)
{
	if(sMatch.phase == PHASE_NONE)
		return;
	Freeze(false);
	sMatch.phase = PHASE_NONE;
}

static void
BeginMatch(const SMatchStart &m, int index)
{
	memset(&sMatch, 0, sizeof(sMatch));
	sMatch.start = m;
	sMatch.myTeam = m.teams[index];
	sMatch.phase = PHASE_COUNTDOWN;
	sMatch.countdownShown = 4;
	memset(&sScores, 0, sizeof(sScores));
	CFreeroamMenu::Close();
	SpawnForMatch();
	Freeze(true);
	char line[96];
	if(m.mode == MATCH_TDM)
		snprintf(line, sizeof(line), "%s: you're on %s. First team to %u wins", ModeName(m.mode),
			sMatch.myTeam == MP_TEAM_BLUE ? "Blue" : "Red", (unsigned)m.scoreLimit);
	else
		snprintf(line, sizeof(line), "%s: first to %u kills wins", ModeName(m.mode), (unsigned)m.scoreLimit);
	CNetChat::AddLine(line, CNetChat::NOTICE);
}

// ---- Host

static void
HostRequest(uint8 from, uint8 mode, uint32 now)
{
	if(CNetRace::HostBusy() && !sHost.IsBusy()){
		SMatchCancel busy;
		busy.matchId = sHost.MatchId();
		busy.reason = MATCH_BUSY;
		sSink.SendCancel(busy, from);
		return;
	}
	sHost.Request(from, mode, true, now, &sSink);
}

void
CNetMatch::OnDied(uint8 victim, uint8 killer)
{
	if(CNetGame::IsHost())
		sHost.OnDeath(victim, killer, CNetGame::SessionNow());
}

void
CNetMatch::OnPlayerJoined(uint8 id)
{
	SMatchInvite invite;
	if(CNetGame::IsHost() && sHost.GetInvite(invite)){
		CByteWriter w;
		invite.Write(w);
		CNetGame::Client().SendGame(MP_ROUTE_PLAYER, id, true, w);
	}
}

void
CNetMatch::OnPlayerLeft(uint8 id)
{
	if(CNetGame::IsHost())
		sHost.OnLeft(id);
	// The relay reuses ids: a newcomer with this one isn't in our match.
	for(int i = 0; i < sMatch.start.n; i++)
		if(sMatch.start.ids[i] == id){
			sMatch.start.ids[i] = MP_PLAYER_NONE;
			sMatch.start.teams[i] = MP_TEAM_NONE;
		}
}

// ---- Per frame

void
CNetMatch::Process(void)
{
	uint32 now = CNetGame::SessionNow();
	if(CNetGame::IsHost())
		sHost.Update(now, &sSink);

	if(sInvite.open && (int32)(now - sInvite.invite.deadline) >= 0)
		sInvite.open = false;
	if(sInvite.open && !sInvite.accepted && sMatch.phase == PHASE_NONE && !CNetRace::IsRacing() &&
	   !CNetChat::IsTyping() && CPad::GetPad(0)->GetFJustDown(2)){	// F3
		SMatchAccept msg;
		msg.matchId = sInvite.invite.matchId;
		CByteWriter w;
		msg.Write(w);
		CNetGame::Client().SendGame(MP_ROUTE_HOST, 0, true, w);
		sInvite.accepted = true;
		CNetChat::AddLine("You're in the match", CNetChat::NOTICE);
	}

	CPlayerPed *player = FindPlayerPed();
	switch(sMatch.phase){
	case PHASE_COUNTDOWN: {
		int32 toGo = (int32)(sMatch.start.startTime - now);
		int shown = toGo > 2000 ? 3 : toGo > 1000 ? 2 : toGo > 0 ? 1 : 0;
		if(toGo <= 3000 && shown < sMatch.countdownShown){
			static const int16 sounds[4] = { SOUND_RACE_START_GO, SOUND_RACE_START_1, SOUND_RACE_START_2, SOUND_RACE_START_3 };
			DMAudio.PlayFrontEndSound(sounds[shown], 0);
			sMatch.countdownShown = shown;
		}
		if(toGo <= 0){
			Freeze(false);
			sMatch.phase = PHASE_PLAYING;
		}else
			Freeze(true);	// a respawn re-enables every control
		break;
	}
	case PHASE_PLAYING:
		break;
	default:
		break;
	}
}

// ---- Messages

void
CNetMatch::OnMessage(uint8 from, uint8 type, CByteReader &body)
{
	uint8 me = CNetGame::MyId();
	uint32 now = CNetGame::SessionNow();
	switch(type){
	case GMSG_MATCH_REQUEST: {
		SMatchRequest m;
		if(CNetGame::IsHost() && m.Read(body))
			HostRequest(from, m.mode, now);
		break;
	}
	case GMSG_MATCH_ACCEPT: {
		SMatchAccept m;
		if(CNetGame::IsHost() && m.Read(body))
			sHost.OnAccept(from, m.matchId, now);
		break;
	}
	case GMSG_MATCH_INVITE: {
		SMatchInvite m;
		if(from != 0 || !m.Read(body))
			return;
		if(sInvite.open && sInvite.invite.matchId == m.matchId)
			return;	// a resend for a late joiner
		sInvite.open = true;
		sInvite.accepted = m.startedBy == me;
		sInvite.invite = m;
		char line[128];
		if(m.startedBy == me)
			snprintf(line, sizeof(line), "You started a %s - others have 15s to join", ModeName(m.mode));
		else
			snprintf(line, sizeof(line), "%s started a %s - press F3 to join (15s)", NameOf(m.startedBy), ModeName(m.mode));
		CNetChat::AddLine(line, CNetChat::NOTICE);
		break;
	}
	case GMSG_MATCH_START: {
		SMatchStart m;
		if(from != 0 || !m.Read(body))
			return;
		sInvite.open = false;
		memset(sNames, 0, sizeof(sNames));
		for(int i = 0; i < m.n; i++)
			if(m.ids[i] < MP_MAX_PLAYERS && CNetGame::PlayerName(m.ids[i]))
				strcpy(sNames[m.ids[i]], CNetGame::PlayerName(m.ids[i]));
		for(int i = 0; i < m.n; i++)
			if(m.ids[i] == me && !CNetRace::IsRacing())
				BeginMatch(m, i);
		break;
	}
	case GMSG_MATCH_SCORES: {
		SMatchScores m;
		if(from == 0 && m.Read(body) && !(sHaveFinished && m.matchId == sLastFinishedMatch))
			sScores = m;
		break;
	}
	case GMSG_MATCH_END: {
		SMatchScores m;
		if(from != 0 || !m.Read(body))
			return;
		bool mine = sMatch.phase != PHASE_NONE && sMatch.start.matchId == m.matchId;
		uint8 mode = mine ? sMatch.start.mode : (m.n && m.entries[0].team != MP_TEAM_NONE ? (uint8)MATCH_TDM : (uint8)MATCH_DM);
		char line[128];
		if(mode == MATCH_TDM){
			int16 r = m.teamScore[MP_TEAM_RED], b = m.teamScore[MP_TEAM_BLUE];
			uint8 winner = m.winnerTeam;
			bool forfeit = winner != MP_TEAM_NONE && (winner == MP_TEAM_RED ? r <= b : b <= r);
			if(winner == MP_TEAM_NONE)
				snprintf(line, sizeof(line), "Team Deathmatch: a draw, %d-%d", (int)r, (int)b);
			else
				snprintf(line, sizeof(line), "%s team won the Team Deathmatch %d-%d (Red-Blue)%s",
					winner == MP_TEAM_BLUE ? "Blue" : "Red", (int)r, (int)b, forfeit ? ", the other team left" : "");
		}else if(m.n)
			snprintf(line, sizeof(line), "%s won the Deathmatch with %d", NameOf(m.entries[0].id), (int)m.entries[0].score);
		else
			strcpy(line, "The Deathmatch is over");
		CNetChat::AddLine(line, CNetChat::NOTICE);
		if(mine){
			sResults = m;
			sResultsMode = mode;
			sResultsUntil = CNetGame::NetNow() + RESULTS_SHOW_MS;
			LeaveMatch();
		}
		memset(&sScores, 0, sizeof(sScores));
		sLastFinishedMatch = m.matchId;
		sHaveFinished = true;
		break;
	}
	case GMSG_MATCH_CANCEL: {
		SMatchCancel m;
		if(from != 0 || !m.Read(body))
			return;
		const char *why = m.reason == MATCH_BUSY ? "A match or race is already on" :
			m.reason == MATCH_ABORTED ? "The match was called off" : "Not enough players joined the match";
		CNetChat::AddLine(why, CNetChat::NOTICE);
		if(m.reason != MATCH_BUSY && sInvite.invite.matchId == m.matchId)
			sInvite.open = false;
		if(m.reason == MATCH_ABORTED && sMatch.phase != PHASE_NONE && sMatch.start.matchId == m.matchId)
			LeaveMatch();
		if(m.reason != MATCH_BUSY){
			memset(&sScores, 0, sizeof(sScores));
			sLastFinishedMatch = m.matchId;
			sHaveFinished = true;
		}
		break;
	}
	default:
		break;
	}
}

void
CNetMatch::Reset(void)
{
	if(CNetGame::IsHost() && sHost.IsBusy()){
		SMatchCancel cancel;
		cancel.matchId = sHost.MatchId();
		cancel.reason = MATCH_ABORTED;
		sSink.SendCancel(cancel, MP_PLAYER_NONE);
	}
	LeaveMatch();
	sHost.Reset();
	memset(&sInvite, 0, sizeof(sInvite));
	memset(&sScores, 0, sizeof(sScores));
	sResultsUntil = 0;
}

// ---- HUD

static void
SetupFont(float sx, float sy, int16 style)
{
	CFont::SetPropOn();
	CFont::SetBackgroundOff();
	CFont::SetJustifyOff();
	CFont::SetCentreOff();
	CFont::SetRightJustifyOff();
	CFont::SetWrapx(SCREEN_WIDTH);
	CFont::SetFontStyle(FONT_LOCALE(style));
	CFont::SetScale(SCREEN_SCALE_X(sx), SCREEN_SCALE_Y(sy));
	CFont::SetDropShadowPosition(1);
	CFont::SetDropColor(CRGBA(0, 0, 0, 255));
}

static const SMatchScoreEntry*
FindEntry(const SMatchScores &s, uint8 id, int *place)
{
	for(int i = 0; i < s.n; i++)
		if(s.entries[i].id == id){
			if(place)
				*place = i + 1;
			return &s.entries[i];
		}
	return nil;
}

static void
DrawBoard(const SMatchScores &s, uint8 mode, const char *title, float y)
{
	float x = SCREEN_WIDTH / 2 - SCREEN_SCALE_X(200.0f);
	float lh = SCREEN_SCALE_Y(22.0f);
	int rows = Max(1, (int)s.n);
	CSprite2d::DrawRect(CRect(x - SCREEN_SCALE_X(15.0f), y - SCREEN_SCALE_Y(12.0f), x + SCREEN_SCALE_X(415.0f),
		y + lh * (rows + 2.5f)), CRGBA(0, 0, 0, 180));
	char line[96];
	SetupFont(0.65f, 1.15f, FONT_HEADING);
	CFont::SetColor(CRGBA(255, 200, 40, 255));
	if(mode == MATCH_TDM)
		snprintf(line, sizeof(line), "%s   Red %d - Blue %d", title, (int)s.teamScore[MP_TEAM_RED], (int)s.teamScore[MP_TEAM_BLUE]);
	else
		snprintf(line, sizeof(line), "%s", title);
	CFont::PrintString(x, y, MpWideString(line));
	y += lh * 1.4f;
	SetupFont(0.45f, 0.85f, FONT_STANDARD);
	CFont::SetColor(CRGBA(180, 180, 180, 255));
	CFont::PrintString(x, y, MpWideString("     Name                           Score   Kills  Deaths"));
	y += lh;
	for(int i = 0; i < s.n; i++){
		const SMatchScoreEntry &e = s.entries[i];
		snprintf(line, sizeof(line), "%d.  %-28s %5d   %5u  %5u", i + 1, NameOf(e.id), (int)e.score, (unsigned)e.kills, (unsigned)e.deaths);
		CFont::SetColor(mode == MATCH_TDM ? TeamRGBA(e.team) : CRemotePlayer::ColourForId(e.id));
		CFont::PrintString(x, y, MpWideString(line));
		y += lh;
	}
}

void
CNetMatch::Render(void)
{
	uint32 now = CNetGame::SessionNow();
	char line[96];

	if(sInvite.open && sMatch.phase == PHASE_NONE){
		int secs = Max(0, (int32)(sInvite.invite.deadline - now) / 1000);
		snprintf(line, sizeof(line), sInvite.accepted ? "%s: starting in %ds" : "%s: F3 to join (%ds)", ModeName(sInvite.invite.mode), secs);
		SetupFont(0.45f, 0.8f, FONT_STANDARD);
		CFont::SetRightJustifyOn();
		CFont::SetColor(CRGBA(255, 220, 120, 255));
		CFont::PrintString(SCREEN_WIDTH - SCREEN_SCALE_X(20.0f), SCREEN_SCALE_Y(140.0f), MpWideString(line));
		CFont::SetRightJustifyOff();
	}

	if(sMatch.phase != PHASE_NONE){
		int32 toGo = (int32)(sMatch.start.startTime - now);
		if(toGo <= 3000 && toGo > -1000){
			const char *text = toGo > 2000 ? "3" : toGo > 1000 ? "2" : toGo > 0 ? "1" : "FIGHT!";
			SetupFont(3.0f, 5.0f, FONT_HEADING);
			CFont::SetCentreOn();
			CFont::SetCentreSize(SCREEN_WIDTH);
			CFont::SetColor(toGo > 0 ? CRGBA(255, 200, 40, 255) : CRGBA(255, 80, 80, 255));
			CFont::PrintString(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - SCREEN_SCALE_Y(60.0f), MpWideString(text));
			CFont::SetCentreOff();
		}

		SetupFont(0.6f, 1.1f, FONT_HEADING);
		CFont::SetRightJustifyOn();
		float x = SCREEN_WIDTH - SCREEN_SCALE_X(20.0f);
		float y = SCREEN_SCALE_Y(150.0f);
		float lh = SCREEN_SCALE_Y(26.0f);
		int place = 0;
		const SMatchScoreEntry *mine = FindEntry(sScores, CNetGame::MyId(), &place);
		if(sMatch.start.mode == MATCH_TDM){
			CFont::SetColor(TeamRGBA(MP_TEAM_RED));
			snprintf(line, sizeof(line), "RED %d", (int)sScores.teamScore[MP_TEAM_RED]);
			CFont::PrintString(x, y, MpWideString(line));
			y += lh;
			CFont::SetColor(TeamRGBA(MP_TEAM_BLUE));
			snprintf(line, sizeof(line), "BLUE %d", (int)sScores.teamScore[MP_TEAM_BLUE]);
			CFont::PrintString(x, y, MpWideString(line));
			y += lh;
		}else{
			CFont::SetColor(CRGBA(255, 255, 255, 255));
			snprintf(line, sizeof(line), "SCORE %d/%u", mine ? (int)mine->score : 0, (unsigned)sMatch.start.scoreLimit);
			CFont::PrintString(x, y, MpWideString(line));
			y += lh;
			if(place){
				snprintf(line, sizeof(line), "POS %d/%d", place, (int)sScores.n);
				CFont::PrintString(x, y, MpWideString(line));
				y += lh;
			}
		}
		CFont::SetColor(CRGBA(255, 255, 255, 255));
		int32 left = (int32)(sMatch.start.startTime + sMatch.start.timeLimitMs - now);
		uint32 secs = left > 0 ? (uint32)left / 1000 : 0;
		snprintf(line, sizeof(line), "TIME %02u:%02u", (unsigned)(secs / 60), (unsigned)(secs % 60));
		CFont::PrintString(x, y, MpWideString(line));
		CFont::SetRightJustifyOff();
	}

	if(sResultsUntil && (int32)(sResultsUntil - CNetGame::NetNow()) > 0)
		DrawBoard(sResults, sResultsMode, sResultsMode == MATCH_TDM ? "TEAM DEATHMATCH" : "DEATHMATCH", SCREEN_SCALE_Y(120.0f));
	CFont::SetDropShadowPosition(0);
}

void
CNetMatch::RenderScoreboard(void)
{
	if(!CPad::NewKeyState.TAB || CNetChat::IsTyping())
		return;
	// A match in progress: its scores. Otherwise just who's here.
	if(sScores.n > 0){
		DrawBoard(sScores, sScores.entries[0].team == MP_TEAM_NONE ? MATCH_DM : MATCH_TDM, "SCORES", SCREEN_SCALE_Y(110.0f));
		CFont::SetDropShadowPosition(0);
		return;
	}
	float x = SCREEN_WIDTH / 2 - SCREEN_SCALE_X(150.0f);
	float y = SCREEN_SCALE_Y(110.0f);
	float lh = SCREEN_SCALE_Y(22.0f);
	int n = CNetGame::NumPlayers();
	CSprite2d::DrawRect(CRect(x - SCREEN_SCALE_X(15.0f), y - SCREEN_SCALE_Y(12.0f), x + SCREEN_SCALE_X(315.0f),
		y + lh * (n + 2)), CRGBA(0, 0, 0, 180));
	char line[64];
	SetupFont(0.65f, 1.15f, FONT_HEADING);
	CFont::SetColor(CRGBA(255, 200, 40, 255));
	snprintf(line, sizeof(line), "%s  %d/%d", CNetGame::InviteCode(), n, CNetGame::MaxPlayers());
	CFont::PrintString(x, y, MpWideString(line));
	y += lh * 1.4f;
	SetupFont(0.5f, 0.9f, FONT_STANDARD);
	for(int id = 0; id < MP_MAX_PLAYERS; id++){
		const char *name = CNetGame::PlayerName((uint8)id);
		if(name == nil || (id == 0 && CNetGame::IsDedicated()))
			continue;
		snprintf(line, sizeof(line), "%s%s%s", name, id == 0 ? "  (host)" : "", id == CNetGame::MyId() ? "  (you)" : "");
		CFont::SetColor(CRemotePlayer::ColourForId((uint8)id));
		CFont::PrintString(x, y, MpWideString(line));
		y += lh;
	}
	CFont::SetDropShadowPosition(0);
}

#endif
