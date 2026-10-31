#include "common.h"

#ifdef MULTIPLAYER

#include "NetGame.h"

static CNetClient sClient;

void
CNetGame::Init(void)
{
}

void
CNetGame::Shutdown(void)
{
	sClient.Leave();
}

void
CNetGame::Service(void)
{
}

void
CNetGame::Process(void)
{
}

void
CNetGame::Render2d(void)
{
}

bool
CNetGame::IsActive(void)
{
	return false;
}

bool
CNetGame::IsHost(void)
{
	return false;
}

uint8
CNetGame::MyId(void)
{
	return sClient.MyId();
}

CNetClient&
CNetGame::Client(void)
{
	return sClient;
}

#endif
