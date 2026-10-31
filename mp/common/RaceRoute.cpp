#include "RaceRoute.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
SetError(char *err, uint32_t errSize, const char *fmt, ...)
{
	if(err == nullptr || errSize == 0)
		return;
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(err, errSize, fmt, ap);
	va_end(ap);
}

static bool
IsSpace(char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}

// Splits line (modified in place) into whitespace-separated tokens.
static int
Tokenize(char *line, char **tokens, int maxTokens)
{
	int n = 0;
	char *p = line;
	while(*p){
		while(*p && IsSpace(*p))
			p++;
		if(*p == '\0')
			break;
		if(n == maxTokens)
			return maxTokens + 1;	// too many
		tokens[n++] = p;
		while(*p && !IsSpace(*p))
			p++;
		if(*p)
			*p++ = '\0';
	}
	return n;
}

static bool
ParseFloat(const char *s, float &out)
{
	char *end;
	double v = strtod(s, &end);
	if(end == s || *end != '\0' || !isfinite(v) || v > 1e7 || v < -1e7)
		return false;
	out = (float)v;
	return true;
}

static bool
ParseInt(const char *s, long &out)
{
	char *end;
	long v = strtol(s, &end, 10);
	if(end == s || *end != '\0')
		return false;
	out = v;
	return true;
}

bool
MpParseRoute(const char *text, SRaceRoute &out, char *err, uint32_t errSize)
{
	memset(&out, 0, sizeof(out));
	SetError(err, errSize, "%s", "");
	if(text == nullptr){
		SetError(err, errSize, "missing name");
		return false;
	}

	bool haveType = false, haveLaps = false, haveVehicle = false;
	long laps = 0, vehicle = 0;
	char line[256];
	int lineNo = 0;
	const char *p = text;

	while(*p){
		// One line, at most sizeof(line)-1 characters (the rest is dropped).
		lineNo++;
		uint32_t len = 0;
		while(*p && *p != '\n'){
			if(len < sizeof(line) - 1)
				line[len++] = *p;
			p++;
		}
		if(*p == '\n')
			p++;
		line[len] = '\0';
		char *hash = strchr(line, '#');
		if(hash)
			*hash = '\0';

		// "name" takes the rest of the line, spaces included.
		char *start = line;
		while(*start && IsSpace(*start))
			start++;
		if(strncmp(start, "name", 4) == 0 && (start[4] == '\0' || IsSpace(start[4]))){
			char *name = start + 4;
			while(*name && IsSpace(*name))
				name++;
			size_t n = strlen(name);
			while(n > 0 && IsSpace(name[n - 1]))
				name[--n] = '\0';
			strncpy(out.name, name, MP_MAX_ROUTE_NAME);
			out.name[MP_MAX_ROUTE_NAME] = '\0';
			continue;
		}

		char *tok[8];
		int n = Tokenize(start, tok, 6);
		if(n == 0)
			continue;
		const char *cmd = tok[0];
		if(strcmp(cmd, "type") == 0){
			if(n != 2)
				goto badNumber;
			if(strcmp(tok[1], "sprint") == 0)
				out.type = RACE_SPRINT;
			else if(strcmp(tok[1], "circuit") == 0)
				out.type = RACE_CIRCUIT;
			else{
				SetError(err, errSize, "bad type");
				return false;
			}
			haveType = true;
		}else if(strcmp(cmd, "laps") == 0){
			if(n != 2 || !ParseInt(tok[1], laps))
				goto badNumber;
			haveLaps = true;
		}else if(strcmp(cmd, "vehicle") == 0){
			if(n != 2 || !ParseInt(tok[1], vehicle))
				goto badNumber;
			haveVehicle = true;
		}else if(strcmp(cmd, "grid") == 0){
			SRaceGridSlot slot;
			if(n != 5 || !ParseFloat(tok[1], slot.pos[0]) || !ParseFloat(tok[2], slot.pos[1]) ||
			   !ParseFloat(tok[3], slot.pos[2]) || !ParseFloat(tok[4], slot.heading))
				goto badNumber;
			if(out.numGrid < MP_MAX_PLAYERS)
				out.grid[out.numGrid++] = slot;
		}else if(strcmp(cmd, "cp") == 0){
			SRaceCheckpoint cp;
			if(n != 5 || !ParseFloat(tok[1], cp.pos[0]) || !ParseFloat(tok[2], cp.pos[1]) ||
			   !ParseFloat(tok[3], cp.pos[2]) || !ParseFloat(tok[4], cp.radius) || cp.radius <= 0.0f)
				goto badNumber;
			if(out.numCheckpoints >= MP_MAX_CHECKPOINTS){
				SetError(err, errSize, "too many checkpoints");
				return false;
			}
			out.cps[out.numCheckpoints++] = cp;
		}else{
			SetError(err, errSize, "line %d: unknown directive '%s'", lineNo, cmd);
			return false;
		}
		continue;
	badNumber:
		SetError(err, errSize, "line %d: bad number", lineNo);
		return false;
	}

	if(out.name[0] == '\0'){
		SetError(err, errSize, "missing name");
		return false;
	}
	if(!haveType){
		SetError(err, errSize, "bad type");
		return false;
	}
	if(out.type == RACE_CIRCUIT){
		if(!haveLaps || laps < 1 || laps > 9){
			SetError(err, errSize, "bad laps");
			return false;
		}
		out.laps = (uint8_t)laps;
	}else
		out.laps = 1;
	if(!haveVehicle || vehicle <= 0 || vehicle > 65535){
		SetError(err, errSize, "bad vehicle");
		return false;
	}
	out.vehicleModel = (uint16_t)vehicle;
	if(out.numGrid == 0){
		SetError(err, errSize, "no grid");
		return false;
	}
	if(out.numCheckpoints < 2){
		SetError(err, errSize, "too few checkpoints");
		return false;
	}
	return true;
}

uint16_t
MpRouteTotalCrossings(const SRaceRoute &route)
{
	if(route.type == RACE_CIRCUIT)
		return (uint16_t)(route.numCheckpoints * route.laps);
	return route.numCheckpoints;
}
