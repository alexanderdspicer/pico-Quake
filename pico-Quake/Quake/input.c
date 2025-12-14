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

//Remove all checks, not needed
static qboolean windowhasfocus = true;	//just in case sdl fails to tell us...
static qboolean	textmode;

static cvar_t in_debugkeys =        {"in_debugkeys", "0", CVAR_NONE};

// SDL2 Game Controller cvars
cvar_t	joy_deadzone_look = 		{ "joy_deadzone_look", "0.175", CVAR_ARCHIVE };
cvar_t	joy_deadzone_move = 		{ "joy_deadzone_move", "0.175", CVAR_ARCHIVE };
cvar_t	joy_outer_threshold_look = 	{ "joy_outer_threshold_look", "0.02", CVAR_ARCHIVE };
cvar_t	joy_outer_threshold_move = 	{ "joy_outer_threshold_move", "0.02", CVAR_ARCHIVE };
cvar_t	joy_deadzone_trigger = 		{ "joy_deadzone_trigger", "0.2", CVAR_ARCHIVE };
cvar_t	joy_sensitivity_yaw = 		{ "joy_sensitivity_yaw", "240", CVAR_ARCHIVE };
cvar_t	joy_sensitivity_pitch = 	{ "joy_sensitivity_pitch", "130", CVAR_ARCHIVE };
cvar_t	joy_invert = 				{ "joy_invert", "0", CVAR_ARCHIVE };
cvar_t	joy_exponent = 				{ "joy_exponent", "2", CVAR_ARCHIVE };
cvar_t	joy_exponent_move = 		{ "joy_exponent_move", "2", CVAR_ARCHIVE };
cvar_t	joy_swapmovelook = 			{ "joy_swapmovelook", "0", CVAR_ARCHIVE };
cvar_t	joy_enable = 				{ "joy_enable", "1", CVAR_ARCHIVE };

//Dynamically set
static qboolean	no_mouse = false;

/* total accumulated mouse movement since last frame */
static int	total_dx, total_dy = 0;

//Rewrite 😔✊🏻