#pragma once

#ifdef MULTIPLAYER

// Chat commands for authoring race routes in-game. Lines starting with '/'
// are local commands and are never sent:
//   /rec new NAME, /rec type sprint|circuit, /rec laps N, /rec vehicle,
//   /rec grid, /rec cp [radius], /rec undo, /rec save, /pos
class CRouteRecorder
{
public:
	// Returns true if text was a command (handled here, not chat).
	static bool OnCommand(const char *text);
};

#endif
