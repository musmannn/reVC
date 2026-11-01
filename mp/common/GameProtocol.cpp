#include "GameProtocol.h"

#include <string.h>

static void
WriteVec(CByteWriter &w, const float *v, int n)
{
	for(int i = 0; i < n; i++)
		w.WriteF32(v[i]);
}

static bool
ReadVec(CByteReader &r, float *v, int n)
{
	for(int i = 0; i < n; i++)
		if(!r.ReadF32(v[i]))
			return false;
	return true;
}

SPlayerState::SPlayerState(void)
{
	memset(this, 0, sizeof(*this));
}

void
SPlayerState::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_PLAYER_STATE);
	w.WriteU32(time);
	w.WriteU16(skin);
	w.WriteU8(flags);
	WriteVec(w, pos, 3);
	w.WriteF32(heading);
	WriteVec(w, vel, 3);
	w.WriteU8(moveState);
	w.WriteU8(weapon);
	WriteVec(w, aimPos, 3);
	w.WriteU8(health);
	w.WriteU8(armour);
	w.WriteU16(vehicleId);
	w.WriteU8(seat);
}

bool
SPlayerState::Read(CByteReader &r)
{
	return r.ReadU32(time) && r.ReadU16(skin) && r.ReadU8(flags) && ReadVec(r, pos, 3) &&
		r.ReadF32(heading) && ReadVec(r, vel, 3) && r.ReadU8(moveState) && r.ReadU8(weapon) &&
		ReadVec(r, aimPos, 3) && r.ReadU8(health) && r.ReadU8(armour) && r.ReadU16(vehicleId) &&
		r.ReadU8(seat);
}

SVehicleState::SVehicleState(void)
{
	memset(this, 0, sizeof(*this));
	quat[3] = 1.0f;
}

void
SVehicleState::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_VEHICLE_STATE);
	w.WriteU32(time);
	w.WriteU16(vehicleId);
	WriteVec(w, pos, 3);
	WriteVec(w, quat, 4);
	WriteVec(w, vel, 3);
	WriteVec(w, turn, 3);
	w.WriteF32(steer);
	w.WriteF32(gas);
	w.WriteF32(brake);
	w.WriteU8(flags);
	w.WriteU16(health);
}

bool
SVehicleState::Read(CByteReader &r)
{
	return r.ReadU32(time) && r.ReadU16(vehicleId) && ReadVec(r, pos, 3) && ReadVec(r, quat, 4) &&
		ReadVec(r, vel, 3) && ReadVec(r, turn, 3) && r.ReadF32(steer) && r.ReadF32(gas) &&
		r.ReadF32(brake) && r.ReadU8(flags) && r.ReadU16(health);
}

SVehicleCreate::SVehicleCreate(void)
{
	memset(this, 0, sizeof(*this));
	quat[3] = 1.0f;
}

void
SVehicleCreate::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_VEHICLE_CREATE);
	w.WriteU16(vehicleId);
	w.WriteU16(model);
	w.WriteU8(col1);
	w.WriteU8(col2);
	WriteVec(w, pos, 3);
	WriteVec(w, quat, 4);
}

bool
SVehicleCreate::Read(CByteReader &r)
{
	return r.ReadU16(vehicleId) && r.ReadU16(model) && r.ReadU8(col1) && r.ReadU8(col2) &&
		ReadVec(r, pos, 3) && ReadVec(r, quat, 4);
}

void
SVehicleRemove::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_VEHICLE_REMOVE);
	w.WriteU16(vehicleId);
}

bool
SVehicleRemove::Read(CByteReader &r)
{
	return r.ReadU16(vehicleId);
}

void
SVehicleEnter::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_VEHICLE_ENTER);
	w.WriteU16(vehicleId);
	w.WriteU8(seat);
}

bool
SVehicleEnter::Read(CByteReader &r)
{
	return r.ReadU16(vehicleId) && r.ReadU8(seat);
}

void
SVehicleSeat::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_VEHICLE_SEAT);
	w.WriteU16(vehicleId);
	w.WriteU8(playerId);
	w.WriteU8(seat);
	w.WriteU8(accepted);
}

bool
SVehicleSeat::Read(CByteReader &r)
{
	return r.ReadU16(vehicleId) && r.ReadU8(playerId) && r.ReadU8(seat) && r.ReadU8(accepted);
}

void
SVehicleExit::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_VEHICLE_EXIT);
	w.WriteU16(vehicleId);
}

bool
SVehicleExit::Read(CByteReader &r)
{
	return r.ReadU16(vehicleId);
}

void
SDamage::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_DAMAGE);
	w.WriteU8(victimId);
	w.WriteU8(weapon);
	w.WriteF32(amount);
	w.WriteU8(piece);
	w.WriteU8(direction);
}

bool
SDamage::Read(CByteReader &r)
{
	return r.ReadU8(victimId) && r.ReadU8(weapon) && r.ReadF32(amount) && r.ReadU8(piece) && r.ReadU8(direction);
}

void
SDied::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_DIED);
	w.WriteU8(killerId);
	w.WriteU8(weapon);
}

bool
SDied::Read(CByteReader &r)
{
	return r.ReadU8(killerId) && r.ReadU8(weapon);
}

SRespawned::SRespawned(void)
{
	memset(this, 0, sizeof(*this));
}

void
SRespawned::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_RESPAWNED);
	WriteVec(w, pos, 3);
	w.WriteF32(heading);
}

bool
SRespawned::Read(CByteReader &r)
{
	return ReadVec(r, pos, 3) && r.ReadF32(heading);
}

SShot::SShot(void)
{
	memset(this, 0, sizeof(*this));
}

void
SShot::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_SHOT);
	w.WriteU8(weapon);
	WriteVec(w, from, 3);
	WriteVec(w, to, 3);
}

bool
SShot::Read(CByteReader &r)
{
	return r.ReadU8(weapon) && ReadVec(r, from, 3) && ReadVec(r, to, 3);
}

void
SWorldState::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_WORLD);
	w.WriteU8(hour);
	w.WriteU8(minute);
	w.WriteU8(weather);
}

bool
SWorldState::Read(CByteReader &r)
{
	return r.ReadU8(hour) && r.ReadU8(minute) && r.ReadU8(weather);
}

uint8_t
MpNextVehicleOwner(uint8_t currentOwner, const uint8_t seats[MP_MAX_SEATS], uint8_t leavingPlayer)
{
	if(leavingPlayer != currentOwner)
		return currentOwner;
	for(int i = 0; i < MP_MAX_SEATS; i++)
		if(seats[i] != MP_PLAYER_NONE && seats[i] != leavingPlayer)
			return seats[i];
	return MP_PLAYER_NONE;
}

void
SRaceRequest::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_RACE_REQUEST);
	w.WriteString(route, MP_MAX_ROUTE_NAME);
}

bool
SRaceRequest::Read(CByteReader &r)
{
	return r.ReadString(route, sizeof(route));
}

void
SRaceInvite::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_RACE_INVITE);
	w.WriteU16(raceId);
	w.WriteU8(startedBy);
	w.WriteString(route, MP_MAX_ROUTE_NAME);
	w.WriteU32(deadline);
}

bool
SRaceInvite::Read(CByteReader &r)
{
	return r.ReadU16(raceId) && r.ReadU8(startedBy) && r.ReadString(route, sizeof(route)) && r.ReadU32(deadline);
}

void
SRaceAccept::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_RACE_ACCEPT);
	w.WriteU16(raceId);
}

bool
SRaceAccept::Read(CByteReader &r)
{
	return r.ReadU16(raceId);
}

void
SRaceStart::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_RACE_START);
	w.WriteU16(raceId);
	w.WriteU32(startTime);
	uint8_t count = n > MP_MAX_PLAYERS ? (uint8_t)MP_MAX_PLAYERS : n;
	w.WriteU8(count);
	w.WriteBytes(ids, count);
}

bool
SRaceStart::Read(CByteReader &r)
{
	if(!r.ReadU16(raceId) || !r.ReadU32(startTime) || !r.ReadU8(n) || n > MP_MAX_PLAYERS)
		return false;
	return r.ReadBytes(ids, n);
}

void
SRaceCross::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_RACE_CROSS);
	w.WriteU16(raceId);
	w.WriteU16(seq);
	w.WriteU32(time);
}

bool
SRaceCross::Read(CByteReader &r)
{
	return r.ReadU16(raceId) && r.ReadU16(seq) && r.ReadU32(time);
}

void
SRaceStandings::Write(CByteWriter &w) const
{
	w.WriteU8(msgType);
	w.WriteU16(raceId);
	uint8_t count = n > MP_MAX_PLAYERS ? (uint8_t)MP_MAX_PLAYERS : n;
	w.WriteU8(count);
	for(uint8_t i = 0; i < count; i++){
		w.WriteU8(entries[i].id);
		w.WriteU16(entries[i].nextSeq);
		w.WriteU8(entries[i].state);
		w.WriteU32(entries[i].finishTime);
	}
}

bool
SRaceStandings::Read(CByteReader &r)
{
	if(!r.ReadU16(raceId) || !r.ReadU8(n) || n > MP_MAX_PLAYERS)
		return false;
	for(uint8_t i = 0; i < n; i++)
		if(!r.ReadU8(entries[i].id) || !r.ReadU16(entries[i].nextSeq) ||
		   !r.ReadU8(entries[i].state) || !r.ReadU32(entries[i].finishTime))
			return false;
	return true;
}

void
SRaceCancel::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_RACE_CANCEL);
	w.WriteU16(raceId);
	w.WriteU8(reason);
}

bool
SRaceCancel::Read(CByteReader &r)
{
	return r.ReadU16(raceId) && r.ReadU8(reason);
}

void
SMatchRequest::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_MATCH_REQUEST);
	w.WriteU8(mode);
}

bool
SMatchRequest::Read(CByteReader &r)
{
	return r.ReadU8(mode);
}

void
SMatchInvite::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_MATCH_INVITE);
	w.WriteU16(matchId);
	w.WriteU8(mode);
	w.WriteU8(startedBy);
	w.WriteU16(scoreLimit);
	w.WriteU32(timeLimitMs);
	w.WriteU32(deadline);
}

bool
SMatchInvite::Read(CByteReader &r)
{
	return r.ReadU16(matchId) && r.ReadU8(mode) && r.ReadU8(startedBy) && r.ReadU16(scoreLimit) &&
		r.ReadU32(timeLimitMs) && r.ReadU32(deadline);
}

void
SMatchAccept::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_MATCH_ACCEPT);
	w.WriteU16(matchId);
}

bool
SMatchAccept::Read(CByteReader &r)
{
	return r.ReadU16(matchId);
}

void
SMatchStart::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_MATCH_START);
	w.WriteU16(matchId);
	w.WriteU8(mode);
	w.WriteU16(scoreLimit);
	w.WriteU32(timeLimitMs);
	w.WriteU32(startTime);
	uint8_t count = n > MP_MAX_PLAYERS ? (uint8_t)MP_MAX_PLAYERS : n;
	w.WriteU8(count);
	for(uint8_t i = 0; i < count; i++){
		w.WriteU8(ids[i]);
		w.WriteU8(teams[i]);
	}
}

bool
SMatchStart::Read(CByteReader &r)
{
	if(!r.ReadU16(matchId) || !r.ReadU8(mode) || !r.ReadU16(scoreLimit) || !r.ReadU32(timeLimitMs) ||
	   !r.ReadU32(startTime) || !r.ReadU8(n) || n > MP_MAX_PLAYERS)
		return false;
	for(uint8_t i = 0; i < n; i++)
		if(!r.ReadU8(ids[i]) || !r.ReadU8(teams[i]))
			return false;
	return true;
}

void
SMatchScores::Write(CByteWriter &w) const
{
	w.WriteU8(msgType);
	w.WriteU16(matchId);
	w.WriteI16(teamScore[0]);
	w.WriteI16(teamScore[1]);
	w.WriteU8(winnerTeam);
	uint8_t count = n > MP_MAX_PLAYERS ? (uint8_t)MP_MAX_PLAYERS : n;
	w.WriteU8(count);
	for(uint8_t i = 0; i < count; i++){
		w.WriteU8(entries[i].id);
		w.WriteU8(entries[i].team);
		w.WriteI16(entries[i].score);
		w.WriteU16(entries[i].kills);
		w.WriteU16(entries[i].deaths);
	}
}

bool
SMatchScores::Read(CByteReader &r)
{
	if(!r.ReadU16(matchId) || !r.ReadI16(teamScore[0]) || !r.ReadI16(teamScore[1]) || !r.ReadU8(winnerTeam) ||
	   !r.ReadU8(n) || n > MP_MAX_PLAYERS)
		return false;
	for(uint8_t i = 0; i < n; i++)
		if(!r.ReadU8(entries[i].id) || !r.ReadU8(entries[i].team) || !r.ReadI16(entries[i].score) ||
		   !r.ReadU16(entries[i].kills) || !r.ReadU16(entries[i].deaths))
			return false;
	return true;
}

void
SMatchCancel::Write(CByteWriter &w) const
{
	w.WriteU8(GMSG_MATCH_CANCEL);
	w.WriteU16(matchId);
	w.WriteU8(reason);
}

bool
SMatchCancel::Read(CByteReader &r)
{
	return r.ReadU16(matchId) && r.ReadU8(reason);
}
