#include "MatchHost.h"

static int32_t
TimeDiff(uint32_t a, uint32_t b)
{
	return (int32_t)(a - b);
}

CMatchHost::CMatchHost(void)
	: m_state(STATE_IDLE), m_matchId(0), m_mode(MATCH_DM), m_startedBy(MP_PLAYER_NONE), m_deadline(0),
	  m_numAccepted(0), m_lastScoresMs(0)
{
}

void
CMatchHost::Reset(void)
{
	m_state = STATE_IDLE;
	m_numAccepted = 0;
}

static uint16_t
LimitFor(uint8_t mode)
{
	return (uint16_t)(mode == MATCH_TDM ? CMatchHost::TDM_SCORE_LIMIT : CMatchHost::DM_SCORE_LIMIT);
}

bool
CMatchHost::Request(uint8_t from, uint8_t mode, bool starterPlays, uint32_t now, IMatchHostSink *sink)
{
	if(IsBusy()){
		SMatchCancel busy;
		busy.matchId = m_matchId;
		busy.reason = MATCH_BUSY;
		sink->SendCancel(busy, from);
		return false;
	}
	m_state = STATE_INVITING;
	m_matchId++;
	m_mode = mode == MATCH_TDM ? (uint8_t)MATCH_TDM : (uint8_t)MATCH_DM;
	m_startedBy = from;
	m_deadline = now + INVITE_MS;
	m_numAccepted = 0;
	if(starterPlays)
		m_accepted[m_numAccepted++] = from;
	SMatchInvite invite;
	GetInvite(invite);
	sink->SendInvite(invite);
	return true;
}

bool
CMatchHost::GetInvite(SMatchInvite &out) const
{
	if(m_state != STATE_INVITING)
		return false;
	out.matchId = m_matchId;
	out.mode = m_mode;
	out.startedBy = m_startedBy;
	out.scoreLimit = LimitFor(m_mode);
	out.timeLimitMs = TIME_LIMIT_MS;
	out.deadline = m_deadline;
	return true;
}

void
CMatchHost::OnAccept(uint8_t from, uint16_t matchId, uint32_t now)
{
	if(m_state != STATE_INVITING || matchId != m_matchId || TimeDiff(now, m_deadline) >= 0)
		return;
	for(int i = 0; i < m_numAccepted; i++)
		if(m_accepted[i] == from)
			return;
	if(m_numAccepted < (int)MP_MAX_PLAYERS)
		m_accepted[m_numAccepted++] = from;
}

void
CMatchHost::OnDeath(uint8_t victim, uint8_t killer, uint32_t time)
{
	if(m_state == STATE_RUNNING)
		m_match.OnDeath(victim, killer, time);
}

void
CMatchHost::OnLeft(uint8_t id)
{
	if(m_state == STATE_RUNNING)
		m_match.OnLeft(id);
	else if(m_state == STATE_INVITING)
		for(int i = 0; i < m_numAccepted; i++)
			if(m_accepted[i] == id){
				m_accepted[i] = m_accepted[--m_numAccepted];
				break;
			}
}

void
CMatchHost::FillScores(SMatchScores &msg) const
{
	uint8_t ids[MP_MAX_PLAYERS];
	int n = m_match.GetStandings(ids);
	msg.matchId = m_matchId;
	msg.teamScore[MP_TEAM_RED] = m_match.TeamScore(MP_TEAM_RED);
	msg.teamScore[MP_TEAM_BLUE] = m_match.TeamScore(MP_TEAM_BLUE);
	msg.winnerTeam = m_match.WinnerTeam();
	msg.n = (uint8_t)n;
	for(int i = 0; i < n; i++){
		const SMatchPlayer *p = m_match.Get(ids[i]);
		msg.entries[i].id = p->id;
		msg.entries[i].team = p->team;
		msg.entries[i].score = p->score;
		msg.entries[i].kills = p->kills;
		msg.entries[i].deaths = p->deaths;
	}
}

void
CMatchHost::Update(uint32_t now, IMatchHostSink *sink)
{
	if(m_state == STATE_INVITING){
		if(TimeDiff(now, m_deadline) < 0)
			return;
		if(m_numAccepted < 2){
			SMatchCancel cancel;
			cancel.matchId = m_matchId;
			cancel.reason = MATCH_NOT_ENOUGH;
			sink->SendCancel(cancel, MP_PLAYER_NONE);
			m_state = STATE_IDLE;
			return;
		}
		SMatchStart start;
		start.matchId = m_matchId;
		start.mode = m_mode;
		start.scoreLimit = LimitFor(m_mode);
		start.timeLimitMs = TIME_LIMIT_MS;
		start.startTime = m_deadline + COUNTDOWN_MS;
		start.n = (uint8_t)m_numAccepted;
		for(int i = 0; i < m_numAccepted; i++)
			start.ids[i] = m_accepted[i];
		if(m_mode == MATCH_TDM)
			CMatchState::AssignTeams(m_numAccepted, start.teams);
		else
			for(int i = 0; i < m_numAccepted; i++)
				start.teams[i] = MP_TEAM_NONE;
		m_match.Start(m_mode, start.ids, start.teams, start.n, start.scoreLimit, start.timeLimitMs, start.startTime);
		m_state = STATE_RUNNING;
		m_lastScoresMs = now;
		sink->SendStart(start);
		return;
	}
	if(m_state != STATE_RUNNING)
		return;
	m_match.Update(now);
	if(m_match.IsOver()){
		SMatchScores end;
		end.msgType = GMSG_MATCH_END;
		FillScores(end);
		sink->SendEnd(end);
		m_state = STATE_IDLE;
		return;
	}
	if(TimeDiff(now, m_lastScoresMs) >= (int32_t)SCORES_MS){
		m_lastScoresMs = now;
		SMatchScores scores;
		FillScores(scores);
		sink->SendScores(scores);
	}
}
