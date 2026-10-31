#pragma once

// Game messages (types 64..95) carried opaquely by CNetClient::SendGame and
// INetClientListener::OnGameMessage. Engine-free so they're unit tested and
// usable by the fake client.
//
// Units: positions in game units (metres), velocities in game units per
// MILLISECOND, quats (x, y, z, w), headings in radians, times in session ms.

#include <stdint.h>

#include "MpTypes.h"
#include "ByteStream.h"
#include "Protocol.h"

enum eGameMsg : uint8_t
{
	GMSG_PLAYER_STATE = 64,		// unreliable, all-but-self
	GMSG_VEHICLE_STATE = 65,	// unreliable, all-but-self, from the owner
	GMSG_VEHICLE_CREATE = 66,	// reliable, all-but-self
	GMSG_VEHICLE_REMOVE = 67,	// reliable
	GMSG_VEHICLE_ENTER = 68,	// reliable -> host
	GMSG_VEHICLE_SEAT = 69,		// reliable, host -> all
	GMSG_VEHICLE_EXIT = 70,		// reliable -> host
	GMSG_DAMAGE = 71,		// reliable -> victim
	GMSG_DIED = 72,			// reliable, all
	GMSG_RESPAWNED = 73,		// reliable, all
	GMSG_SHOT = 74,			// unreliable, all-but-self
	GMSG_WORLD = 75,		// reliable, host -> all

	GMSG_RACE_FIRST = 80,		// 80..95 reserved for races (Plan 3)
	GMSG_LAST = 95
};

static_assert((uint32_t)GMSG_PLAYER_STATE == (uint32_t)MSG_GAME_FIRST, "game messages start at MSG_GAME_FIRST");

// SPlayerState::flags
enum
{
	PSF_IN_VEHICLE = 1,
	PSF_DEAD = 2,
	PSF_AIMING = 4,
	PSF_FIRING = 8,
	PSF_CROUCH = 16,
	PSF_JUMPING = 32
};

// SVehicleState::flags
enum
{
	VSF_HANDBRAKE = 1,
	VSF_SIREN = 2,
	VSF_LIGHTS = 4,
	VSF_ENGINE = 8
};

static const uint8_t MP_SEAT_DRIVER = 0;	// 1..3 are passengers
static const uint8_t MP_MAX_SEATS = 4;
static const uint8_t MP_SEAT_NONE = 0xFF;	// "left the vehicle"
static const uint16_t MP_VEHICLE_NONE = 0;

// Vehicle ids are (owner << 12) | counter, counter 1..4095, so every player
// can mint ids without asking the host.
inline uint16_t MpMakeVehicleId(uint8_t owner, uint16_t counter) { return (uint16_t)(((owner & 0xF) << 12) | (counter & 0xFFF)); }
inline uint8_t MpVehicleIdOwner(uint16_t id) { return (uint8_t)(id >> 12); }
inline uint16_t MpVehicleIdCounter(uint16_t id) { return (uint16_t)(id & 0xFFF); }

struct SPlayerState
{
	uint32_t time;
	uint16_t skin;		// model id; 0 = default by player id
	uint8_t flags;		// PSF_*
	float pos[3];
	float heading;		// radians
	float vel[3];		// units per ms
	uint8_t moveState;	// engine eMoveState
	uint8_t weapon;		// engine eWeaponType
	float aimPos[3];
	uint8_t health;
	uint8_t armour;
	uint16_t vehicleId;	// with PSF_IN_VEHICLE
	uint8_t seat;		// with PSF_IN_VEHICLE: 0 driver, 1..3 passengers

	SPlayerState(void);
	void Write(CByteWriter &w) const;
	bool Read(CByteReader &r);
};

struct SVehicleState
{
	uint32_t time;
	uint16_t vehicleId;
	float pos[3];
	float quat[4];
	float vel[3];		// units per ms
	float turn[3];		// engine turn speed, converted like vel
	float steer;
	float gas;
	float brake;
	uint8_t flags;		// VSF_*
	uint16_t health;	// 0..1000

	SVehicleState(void);
	void Write(CByteWriter &w) const;
	bool Read(CByteReader &r);
};

struct SVehicleCreate
{
	uint16_t vehicleId;
	uint16_t model;
	uint8_t col1;
	uint8_t col2;
	float pos[3];
	float quat[4];

	SVehicleCreate(void);
	void Write(CByteWriter &w) const;
	bool Read(CByteReader &r);
};

struct SVehicleRemove
{
	uint16_t vehicleId;

	SVehicleRemove(void) : vehicleId(MP_VEHICLE_NONE) {}
	void Write(CByteWriter &w) const;
	bool Read(CByteReader &r);
};

struct SVehicleEnter
{
	uint16_t vehicleId;
	uint8_t seat;

	SVehicleEnter(void) : vehicleId(MP_VEHICLE_NONE), seat(MP_SEAT_DRIVER) {}
	void Write(CByteWriter &w) const;
	bool Read(CByteReader &r);
};

struct SVehicleSeat
{
	uint16_t vehicleId;
	uint8_t playerId;
	uint8_t seat;		// MP_SEAT_NONE = left the vehicle
	uint8_t accepted;

	SVehicleSeat(void) : vehicleId(MP_VEHICLE_NONE), playerId(MP_PLAYER_NONE), seat(MP_SEAT_NONE), accepted(0) {}
	void Write(CByteWriter &w) const;
	bool Read(CByteReader &r);
};

struct SVehicleExit
{
	uint16_t vehicleId;

	SVehicleExit(void) : vehicleId(MP_VEHICLE_NONE) {}
	void Write(CByteWriter &w) const;
	bool Read(CByteReader &r);
};

struct SDamage
{
	uint8_t victimId;
	uint8_t weapon;
	float amount;
	uint8_t piece;
	uint8_t direction;

	SDamage(void) : victimId(MP_PLAYER_NONE), weapon(0), amount(0.0f), piece(0), direction(0) {}
	void Write(CByteWriter &w) const;
	bool Read(CByteReader &r);
};

struct SDied
{
	uint8_t killerId;	// MP_PLAYER_NONE if none
	uint8_t weapon;

	SDied(void) : killerId(MP_PLAYER_NONE), weapon(0) {}
	void Write(CByteWriter &w) const;
	bool Read(CByteReader &r);
};

struct SRespawned
{
	float pos[3];
	float heading;

	SRespawned(void);
	void Write(CByteWriter &w) const;
	bool Read(CByteReader &r);
};

struct SShot
{
	uint8_t weapon;
	float from[3];
	float to[3];

	SShot(void);
	void Write(CByteWriter &w) const;
	bool Read(CByteReader &r);
};

struct SWorldState
{
	uint8_t hour;
	uint8_t minute;
	uint8_t weather;

	SWorldState(void) : hour(12), minute(0), weather(0) {}
	void Write(CByteWriter &w) const;
	bool Read(CByteReader &r);
};
