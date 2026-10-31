#include "common.h"

#ifdef MULTIPLAYER

#include "RouteRecorder.h"
#include "NetChat.h"
#include "NetRace.h"
#include "RaceRoute.h"
#include "FileMgr.h"
#include "PlayerInfo.h"
#include "PlayerPed.h"
#include "Vehicle.h"
#include "ModelIndices.h"

enum { MAX_LINES = 1 + MP_MAX_PLAYERS + MP_MAX_CHECKPOINTS };

struct SRecorder
{
	bool active;
	char name[MP_MAX_ROUTE_NAME + 1];
	bool circuit;
	int laps;
	int vehicle;
	// Grid and checkpoint lines in the order they were added (for undo).
	char lines[MAX_LINES][64];
	bool isGrid[MAX_LINES];
	int numLines;
};
static SRecorder sRec;

static void
Say(const char *text)
{
	CNetChat::AddLine(text, CNetChat::NOTICE);
}

static bool
PlayerPos(CVector &pos, float &headingDeg, CVehicle **vehicle)
{
	CPlayerPed *player = FindPlayerPed();
	if(player == nil)
		return false;
	CVehicle *v = player->bInVehicle ? player->m_pMyVehicle : nil;
	pos = v ? v->GetPosition() : player->GetPosition();
	float heading = v ? v->GetForward().Heading() : player->m_fRotationCur;
	headingDeg = RADTODEG(heading);
	if(headingDeg < 0.0f)
		headingDeg += 360.0f;
	if(vehicle)
		*vehicle = v;
	return true;
}

static int
CountGrid(void)
{
	int n = 0;
	for(int i = 0; i < sRec.numLines; i++)
		if(sRec.isGrid[i])
			n++;
	return n;
}

// "Ocean Drive Sprint" -> "ocean_drive_sprint"
static void
Slug(const char *name, char *out, int size)
{
	int n = 0;
	for(const char *p = name; *p && n < size - 1; p++){
		char c = *p;
		if(c >= 'A' && c <= 'Z')
			c = (char)(c - 'A' + 'a');
		if((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
			out[n++] = c;
		else if(n > 0 && out[n - 1] != '_')
			out[n++] = '_';
	}
	while(n > 0 && out[n - 1] == '_')
		n--;
	out[n] = '\0';
}

static bool
WriteFile(const char *path, const char *text)
{
	int fd = CFileMgr::OpenFileForWriting(path);
	if(fd == 0)
		return false;
	size_t len = strlen(text);
	bool ok = CFileMgr::Write(fd, text, len) == len;
	CFileMgr::CloseFile(fd);
	return ok;
}

static void
Save(void)
{
	if(!sRec.active || sRec.name[0] == '\0'){
		Say("No route: /rec new NAME first");
		return;
	}
	static char text[16 * 1024];
	int n = snprintf(text, sizeof(text), "# Recorded in-game with /rec\nname %s\ntype %s\nlaps %d\nvehicle %d\n",
		sRec.name, sRec.circuit ? "circuit" : "sprint", sRec.circuit ? sRec.laps : 1, sRec.vehicle);
	for(int pass = 0; pass < 2; pass++)	// grid first, then checkpoints
		for(int i = 0; i < sRec.numLines && n < (int)sizeof(text); i++)
			if(sRec.isGrid[i] == (pass == 0))
				n += snprintf(text + n, sizeof(text) - n, "%s\n", sRec.lines[i]);

	// Check it the same way the game will load it.
	SRaceRoute route;
	char err[128];
	if(!MpParseRoute(text, route, err, sizeof(err))){
		char line[160];
		snprintf(line, sizeof(line), "Not saved: %s", err);
		Say(line);
		return;
	}

	char slug[64], file[80], path[128];
	Slug(sRec.name, slug, sizeof(slug));
	if(slug[0] == '\0')
		strcpy(slug, "route");
	snprintf(file, sizeof(file), "%s.route", slug);
	snprintf(path, sizeof(path), "data\\mp\\races\\%s", file);
	CFileMgr::SetDir("");
	if(!WriteFile(path, text)){
		Say("Not saved: can't write data/mp/races (does the folder exist?)");
		return;
	}

	// Add it to the index unless it's there already.
	static char index[8 * 1024];
	int len = MpReadTextFile("data\\mp\\races\\index.txt", index, sizeof(index) - 128);
	if(len < 0)
		len = 0;
	bool listed = false;
	for(const char *p = index; *p; ){
		const char *end = p;
		while(*end && *end != '\n' && *end != '\r')
			end++;
		if((size_t)(end - p) == strlen(file) && strncmp(p, file, end - p) == 0)
			listed = true;
		p = *end ? end + 1 : end;
	}
	if(!listed){
		if(len > 0 && index[len - 1] != '\n')
			index[len++] = '\n';
		snprintf(index + len, sizeof(index) - len, "%s\n", file);
		if(!WriteFile("data\\mp\\races\\index.txt", index)){
			Say("Saved the route, but couldn't add it to data/mp/races/index.txt");
			return;
		}
	}
	CNetRace::Init();
	char line[160];
	snprintf(line, sizeof(line), "%s %s (%d grid, %d checkpoints)", listed ? "Replaced" : "Saved", file,
		(int)route.numGrid, (int)route.numCheckpoints);
	Say(line);
}

bool
CRouteRecorder::OnCommand(const char *text)
{
	if(text[0] != '/')
		return false;
	char buf[128];
	strncpy(buf, text, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';
	char *cmd = strtok(buf, " ");
	char *sub = strtok(nil, " ");
	char *rest = strtok(nil, "");
	char line[160];
	CVector pos;
	float heading;
	CVehicle *vehicle = nil;

	if(strcmp(cmd, "/pos") == 0){
		if(PlayerPos(pos, heading, nil)){
			snprintf(line, sizeof(line), "%.1f %.1f %.1f  heading %.0f", pos.x, pos.y, pos.z, heading);
			Say(line);
		}
		return true;
	}
	if(strcmp(cmd, "/rec") != 0 || sub == nil){
		Say("Commands: /pos, /rec new|type|laps|vehicle|grid|cp|undo|save");
		return true;
	}

	if(strcmp(sub, "new") == 0){
		memset(&sRec, 0, sizeof(sRec));
		sRec.active = true;
		sRec.laps = 1;
		sRec.vehicle = MI_INFERNUS;	// until /rec vehicle
		strncpy(sRec.name, rest ? rest : "New Route", MP_MAX_ROUTE_NAME);
		snprintf(line, sizeof(line), "Recording '%s'", sRec.name);
		Say(line);
		return true;
	}
	if(!sRec.active){
		Say("Start with /rec new NAME");
		return true;
	}
	if(strcmp(sub, "type") == 0 && rest){
		sRec.circuit = rest[0] == 'c' || rest[0] == 'C';
		Say(sRec.circuit ? "Type: circuit" : "Type: sprint");
	}else if(strcmp(sub, "laps") == 0 && rest){
		sRec.laps = Clamp(atoi(rest), 1, 9);
		snprintf(line, sizeof(line), "Laps: %d", sRec.laps);
		Say(line);
	}else if(strcmp(sub, "vehicle") == 0){
		if(PlayerPos(pos, heading, &vehicle) && vehicle){
			sRec.vehicle = vehicle->GetModelIndex();
			snprintf(line, sizeof(line), "Vehicle: model %d", sRec.vehicle);
			Say(line);
		}else
			Say("Get in the race car first");
	}else if(strcmp(sub, "grid") == 0 || strcmp(sub, "cp") == 0){
		bool grid = sub[0] == 'g';
		if(grid && CountGrid() >= (int)MP_MAX_PLAYERS){
			Say("The grid is full (8 slots)");
			return true;
		}
		if(sRec.numLines >= MAX_LINES || !PlayerPos(pos, heading, nil))
			return true;
		float radius = rest ? (float)atof(rest) : 8.0f;
		if(radius <= 0.0f)
			radius = 8.0f;
		if(grid)
			snprintf(sRec.lines[sRec.numLines], sizeof(sRec.lines[0]), "grid %.2f %.2f %.2f %.1f", pos.x, pos.y, pos.z, heading);
		else
			snprintf(sRec.lines[sRec.numLines], sizeof(sRec.lines[0]), "cp %.2f %.2f %.2f %.1f", pos.x, pos.y, pos.z, radius);
		sRec.isGrid[sRec.numLines] = grid;
		sRec.numLines++;
		snprintf(line, sizeof(line), "Added %s", sRec.lines[sRec.numLines - 1]);
		Say(line);
	}else if(strcmp(sub, "undo") == 0){
		if(sRec.numLines > 0){
			sRec.numLines--;
			snprintf(line, sizeof(line), "Removed %s", sRec.lines[sRec.numLines]);
			Say(line);
		}
	}else if(strcmp(sub, "save") == 0)
		Save();
	else
		Say("Commands: /pos, /rec new|type|laps|vehicle|grid|cp|undo|save");
	return true;
}

#endif
