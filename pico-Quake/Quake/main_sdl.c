/*
    Core 0 is dedicated to both server/client processing and rendering
    where rendering may be supplimented (should Core 1 be available)
    by Core 1
*/

/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2005 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"
#include <stdio.h>

static void Sys_AtExit (void)
{

}

#define DEFAULT_MEMORY (256 * 1024 * 1024) // ericw -- was 72MB (64-bit) / 64MB (32-bit)

static quakeparms_t	parms;

int main()
{
	int		t;
	double	time, oldtime, newtime;

	host_parms = &parms;
	parms.basedir = ".";

	//parms.argc = argc;
	//parms.argv = argv;

	parms.errstate = 0;

	//COM_InitArgv(parms.argc, parms.argv);

	//isDedicated = (COM_CheckParm("-dedicated") != 0);

	//Sys_InitSDL ();

	Sys_Init();

	Sys_Printf("Initializing PicoQuake v%s\n", PICO_QUAKE_VER_STRING);

	parms.memsize = DEFAULT_MEMORY;
	/*if (COM_CheckParm("-heapsize"))
	{
		t = COM_CheckParm("-heapsize") + 1;
		if (t < com_argc)
			parms.memsize = Q_atoi(com_argv[t]) * 1024;
	}*/

	parms.membase = malloc (parms.memsize);

	Sys_Printf("Host_Init\n");
	Host_Init();

	oldtime = Sys_DoubleTime();
#ifdef DEDICATED
	//Dedicated server
	while (1)
	{
		newtime = Sys_DoubleTime ();
		time = newtime - oldtime;

		while (time < sys_ticrate.value )
		{
			SDL_Delay(1);
			newtime = Sys_DoubleTime ();
			time = newtime - oldtime;
		}

		Host_Frame (time);
		oldtime = newtime;
	}
#else
	//Non dedicated environment
	while (1)
	{

		scr_skipupdate = 0;
		newtime = Sys_DoubleTime ();
		time = newtime - oldtime;

		Host_Frame (time);

		if (time < sys_throttle.value && !cls.timedemo)
			SDL_Delay(1);

		oldtime = newtime;
	}
#endif

	return 0;
}
