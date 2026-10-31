#include "doctest.h"

#include <string.h>

#include "GameProtocol.h"

template<typename T>
static bool
RoundTrip(const T &msg, uint8_t expectedType, T &out, uint32_t *size = nullptr)
{
	CByteWriter w;
	msg.Write(w);
	if(w.Overflowed())
		return false;
	if(size)
		*size = w.Size();
	CByteReader r(w.Data(), w.Size());
	uint8_t type = 0;
	if(!r.ReadU8(type) || type != expectedType)
		return false;
	return out.Read(r) && r.Remaining() == 0;
}

template<typename T>
static void
CheckAllPrefixesFail(const T &msg)
{
	CByteWriter w;
	msg.Write(w);
	REQUIRE(!w.Overflowed());
	for(uint32_t len = 1; len < w.Size(); len++){
		CByteReader r(w.Data(), len);
		uint8_t type = 0;
		REQUIRE(r.ReadU8(type));
		T out;
		CAPTURE(len);
		CHECK(!out.Read(r));
	}
}

static void
Fill3(float *v, float a, float b, float c)
{
	v[0] = a; v[1] = b; v[2] = c;
}

TEST_CASE("game message type values")
{
	CHECK((int)GMSG_PLAYER_STATE == (int)MSG_GAME_FIRST);
	CHECK(GMSG_PLAYER_STATE == 64);
	CHECK(GMSG_VEHICLE_STATE == 65);
	CHECK(GMSG_WORLD == 75);
}

TEST_CASE("player state round trip and size")
{
	SPlayerState in, out;
	in.time = 123456;
	in.skin = 7;
	in.flags = PSF_IN_VEHICLE | PSF_AIMING | PSF_JUMPING;
	Fill3(in.pos, 100.5f, -200.25f, 10.0f);
	in.heading = 1.25f;
	Fill3(in.vel, 0.01f, -0.02f, 0.0f);
	in.moveState = 4;
	in.weapon = 17;
	Fill3(in.aimPos, 1.0f, 2.0f, 3.0f);
	in.health = 87;
	in.armour = 42;
	in.vehicleId = MpMakeVehicleId(2, 99);
	in.seat = 1;
	uint32_t size = 0;
	REQUIRE(RoundTrip(in, GMSG_PLAYER_STATE, out, &size));
	CHECK(size <= 64);
	CHECK(out.time == 123456);
	CHECK(out.skin == 7);
	CHECK(out.flags == (PSF_IN_VEHICLE | PSF_AIMING | PSF_JUMPING));
	CHECK(out.pos[0] == 100.5f);
	CHECK(out.pos[1] == -200.25f);
	CHECK(out.pos[2] == 10.0f);
	CHECK(out.heading == 1.25f);
	CHECK(out.vel[0] == 0.01f);
	CHECK(out.vel[1] == -0.02f);
	CHECK(out.moveState == 4);
	CHECK(out.weapon == 17);
	CHECK(out.aimPos[2] == 3.0f);
	CHECK(out.health == 87);
	CHECK(out.armour == 42);
	CHECK(out.vehicleId == in.vehicleId);
	CHECK(out.seat == 1);
	CheckAllPrefixesFail(in);
}

TEST_CASE("vehicle state round trip and size")
{
	SVehicleState in, out;
	in.time = 999;
	in.vehicleId = MpMakeVehicleId(1, 5);
	Fill3(in.pos, 1.0f, 2.0f, 3.0f);
	in.quat[0] = 0.0f; in.quat[1] = 0.0f; in.quat[2] = 0.7071f; in.quat[3] = 0.7071f;
	Fill3(in.vel, 0.015f, 0.0f, 0.0f);
	Fill3(in.turn, 0.0f, 0.0f, 0.001f);
	in.steer = -0.3f;
	in.gas = 1.0f;
	in.brake = 0.25f;
	in.flags = VSF_LIGHTS | VSF_ENGINE;
	in.health = 650;
	uint32_t size = 0;
	REQUIRE(RoundTrip(in, GMSG_VEHICLE_STATE, out, &size));
	CHECK(size <= 80);
	CHECK(out.time == 999);
	CHECK(out.vehicleId == in.vehicleId);
	CHECK(out.pos[1] == 2.0f);
	CHECK(out.quat[2] == 0.7071f);
	CHECK(out.vel[0] == 0.015f);
	CHECK(out.turn[2] == 0.001f);
	CHECK(out.steer == -0.3f);
	CHECK(out.gas == 1.0f);
	CHECK(out.brake == 0.25f);
	CHECK(out.flags == (VSF_LIGHTS | VSF_ENGINE));
	CHECK(out.health == 650);
}

TEST_CASE("truncated vehicle state fails")
{
	SVehicleState in;
	in.vehicleId = 1;
	CheckAllPrefixesFail(in);
}

TEST_CASE("other game messages round trip")
{
	SUBCASE("vehicle create")
	{
		SVehicleCreate in, out;
		in.vehicleId = MpMakeVehicleId(3, 4095);
		in.model = 132;
		in.col1 = 12;
		in.col2 = 34;
		Fill3(in.pos, -5.0f, 6.0f, 7.0f);
		in.quat[3] = 1.0f;
		REQUIRE(RoundTrip(in, GMSG_VEHICLE_CREATE, out));
		CHECK(out.vehicleId == in.vehicleId);
		CHECK(out.model == 132);
		CHECK(out.col1 == 12);
		CHECK(out.col2 == 34);
		CHECK(out.pos[0] == -5.0f);
		CHECK(out.quat[3] == 1.0f);
		CheckAllPrefixesFail(in);
	}
	SUBCASE("vehicle remove")
	{
		SVehicleRemove in, out;
		in.vehicleId = 77;
		REQUIRE(RoundTrip(in, GMSG_VEHICLE_REMOVE, out));
		CHECK(out.vehicleId == 77);
	}
	SUBCASE("vehicle enter")
	{
		SVehicleEnter in, out;
		in.vehicleId = 78;
		in.seat = 2;
		REQUIRE(RoundTrip(in, GMSG_VEHICLE_ENTER, out));
		CHECK(out.vehicleId == 78);
		CHECK(out.seat == 2);
	}
	SUBCASE("vehicle seat")
	{
		SVehicleSeat in, out;
		in.vehicleId = 79;
		in.playerId = 5;
		in.seat = MP_SEAT_NONE;
		in.accepted = 1;
		REQUIRE(RoundTrip(in, GMSG_VEHICLE_SEAT, out));
		CHECK(out.vehicleId == 79);
		CHECK(out.playerId == 5);
		CHECK(out.seat == MP_SEAT_NONE);
		CHECK(out.accepted == 1);
		CheckAllPrefixesFail(in);
	}
	SUBCASE("vehicle exit")
	{
		SVehicleExit in, out;
		in.vehicleId = 80;
		REQUIRE(RoundTrip(in, GMSG_VEHICLE_EXIT, out));
		CHECK(out.vehicleId == 80);
	}
	SUBCASE("damage")
	{
		SDamage in, out;
		in.victimId = 4;
		in.weapon = 23;
		in.amount = 33.5f;
		in.piece = 9;
		in.direction = 2;
		REQUIRE(RoundTrip(in, GMSG_DAMAGE, out));
		CHECK(out.victimId == 4);
		CHECK(out.weapon == 23);
		CHECK(out.amount == 33.5f);
		CHECK(out.piece == 9);
		CHECK(out.direction == 2);
		CheckAllPrefixesFail(in);
	}
	SUBCASE("died")
	{
		SDied in, out;
		in.killerId = MP_PLAYER_NONE;
		in.weapon = 6;
		REQUIRE(RoundTrip(in, GMSG_DIED, out));
		CHECK(out.killerId == MP_PLAYER_NONE);
		CHECK(out.weapon == 6);
	}
	SUBCASE("respawned")
	{
		SRespawned in, out;
		Fill3(in.pos, 10.0f, 20.0f, 30.0f);
		in.heading = 3.0f;
		REQUIRE(RoundTrip(in, GMSG_RESPAWNED, out));
		CHECK(out.pos[2] == 30.0f);
		CHECK(out.heading == 3.0f);
	}
	SUBCASE("shot")
	{
		SShot in, out;
		in.weapon = 17;
		Fill3(in.from, 1.0f, 2.0f, 3.0f);
		Fill3(in.to, 4.0f, 5.0f, 6.0f);
		REQUIRE(RoundTrip(in, GMSG_SHOT, out));
		CHECK(out.weapon == 17);
		CHECK(out.from[1] == 2.0f);
		CHECK(out.to[2] == 6.0f);
		CheckAllPrefixesFail(in);
	}
	SUBCASE("world")
	{
		SWorldState in, out;
		in.hour = 23;
		in.minute = 59;
		in.weather = 4;
		REQUIRE(RoundTrip(in, GMSG_WORLD, out));
		CHECK(out.hour == 23);
		CHECK(out.minute == 59);
		CHECK(out.weather == 4);
	}
}

TEST_CASE("vehicle ids")
{
	CHECK(MpVehicleIdOwner(MpMakeVehicleId(3, 7)) == 3);
	CHECK(MpVehicleIdCounter(MpMakeVehicleId(3, 7)) == 7);
	CHECK(MpVehicleIdOwner(MpMakeVehicleId(7, 4095)) == 7);
	CHECK(MpVehicleIdCounter(MpMakeVehicleId(7, 4095)) == 4095);
	CHECK(MpMakeVehicleId(0, 1) != MP_VEHICLE_NONE);
	// Ids from different owners never collide.
	CHECK(MpMakeVehicleId(1, 5) != MpMakeVehicleId(2, 5));
}
