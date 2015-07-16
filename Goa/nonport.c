/******
nonport.c
GameSpy Developer SDK

Copyright 1999 GameSpy Industries, Inc

Suite E-204
2900 Bristol Street
Costa Mesa, CA 92626
(714)549-7689
Fax(714)549-0757

******/
#include "nonport.h"

#ifndef GAMESPY_HARD_LINKED
/* ... so that q2 engine symbols are not needed */
void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, error);
	Q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	gspyi.Sys_Error (ERR_DROP, "%s", text);
}

void Com_Printf (char *msg, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	Q_vsnprintf (text, sizeof(text), msg, argptr);
	va_end (argptr);

	gspyi.Con_Printf("%s", text);
}

void Q_strncpyz (char *dst, const char *src, int dstSize)
{
	if (!dst) return;
	if (!src) return;
	if (dstSize < 1) return;
	strncpy(dst, src, dstSize-1);
	dst[dstSize-1] = 0;
}

void Com_sprintf (char *dest, int size, char *fmt, ...)
{
	int		len;
	va_list		argptr;
	char	bigbuffer[0x10000];

	va_start (argptr,fmt);
	len = Q_vsnprintf (bigbuffer, sizeof(bigbuffer), fmt, argptr);
	va_end (argptr);
	if (len < 0) {
		Com_Printf ("Com_sprintf: overflow in temp buffer of size %i\n", (int)sizeof(bigbuffer));
	}
	else if (len >= size) {
		Com_Printf ("Com_sprintf: overflow of %i in %i\n", len, size);
	}
	strncpy (dest, bigbuffer, size-1);
	dest[size-1] = 0;
}
#endif /* ! GAMESPY_HARD_LINKED */

unsigned long current_time(void) /* returns current time in msec */
{
#ifndef _WIN32
	struct timeval time;

	gettimeofday(&time, NULL);
	return (time.tv_sec * 1000 + time.tv_usec / 1000);
#else
	return (GetTickCount());
#endif
}

void msleep(unsigned long msec)
{
#ifndef _WIN32
	usleep(msec * 1000);
#else
	Sleep(msec);
#endif
}

void SocketStartUp(void)
{
#ifdef _WIN32
	WSADATA data;
	WSAStartup(0x0101, &data);
#endif
}

void SocketShutDown(void)
{
#ifdef _WIN32
	WSACleanup();
#endif
}
