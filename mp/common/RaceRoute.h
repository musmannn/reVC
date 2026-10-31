#pragma once

// Race routes: an engine-free description plus a parser for .route files.
//
// File format (text, '#' comments, one directive per line, whitespace-separated):
//   name Ocean Drive Sprint
//   type sprint            # or circuit
//   laps 1                 # required for circuit (1..9); sprint forces 1
//   vehicle 132
//   grid  X Y Z HEADING    # 1..8 lines, slot order = grid order (more are ignored)
//   cp    X Y Z RADIUS     # 2..64 lines, driving order; the last cp is the finish
//                          # line (and, for a circuit, the lap line)

#include <stdint.h>

#include "MpTypes.h"

static const uint32_t MP_MAX_CHECKPOINTS = 64;
static const uint32_t MP_MAX_ROUTE_NAME = 31;

enum eRaceType
{
	RACE_SPRINT,
	RACE_CIRCUIT
};

struct SRaceGridSlot
{
	float pos[3];
	float heading;	// degrees, as authored
};

struct SRaceCheckpoint
{
	float pos[3];
	float radius;
};

struct SRaceRoute
{
	char name[MP_MAX_ROUTE_NAME + 1];
	uint8_t type;		// eRaceType
	uint8_t laps;
	uint16_t vehicleModel;
	uint8_t numGrid;
	SRaceGridSlot grid[MP_MAX_PLAYERS];
	uint8_t numCheckpoints;
	SRaceCheckpoint cps[MP_MAX_CHECKPOINTS];
};

// Parses text into out. On failure returns false and writes a reason to err
// starting with one of: "missing name", "bad type", "bad laps", "bad vehicle",
// "no grid", "too few checkpoints", "too many checkpoints",
// "line N: bad number", "line N: unknown directive". On success err is "".
bool MpParseRoute(const char *text, SRaceRoute &out, char *err, uint32_t errSize);

// Checkpoint crossings needed to finish: numCheckpoints, times laps for a circuit.
uint16_t MpRouteTotalCrossings(const SRaceRoute &route);
