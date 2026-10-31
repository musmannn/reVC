#pragma once

// reVC multiplayer: the game side of a session. Owns the CNetClient from
// mp/common and hooks into the main loop. Everything here is inert outside a
// session (IsActive() is false), so single player behaves as before.

#ifdef MULTIPLAYER

#include "NetClient.h"

class CNetGame
{
public:
	static void Init(void);
	static void Shutdown(void);
	// Pumps the network. Called every frame, including while paused.
	static void Service(void);
	// Simulation sync. Called only while the game isn't paused.
	static void Process(void);
	static void Render2d(void);

	static bool IsActive(void);
	static bool IsHost(void);
	static uint8 MyId(void);
	static CNetClient &Client(void);
};

#endif
