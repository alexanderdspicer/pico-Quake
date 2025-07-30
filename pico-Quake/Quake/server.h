/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
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

#ifndef QUAKE_SERVER_H
#define QUAKE_SERVER_H

// server.h

#include "q_stdinc.h"

typedef struct
{
	int			maxclients;
	int			maxclientslimit;
	struct client_s	*clients;		// [maxclients]
	int			serverflags;		// episode completion information
	qboolean	changelevel_issued;	// cleared when at SV_SpawnServer
} server_static_t;

//=============================================================================

#define MAX_SIGNON_BUFFERS 256

//Make 8-bit
typedef enum {ss_loading, ss_active} server_state_t;

typedef struct
{
	qboolean	active;				// false if only a net client				//1
	qboolean	paused;															//1
	qboolean	loadgame;			// handle connections specially				//1

	double		time;															//4

	int			lastcheck;			// used by PF_checkclient					//4
	double		lastchecktime;													//4

	char		name[64];			// map name									//64
	char		modelname[64];		// maps/<name>.bsp, for model_precache[0]	//64
	struct qmodel_s	*worldmodel;												//4
	const char	*model_precache[MAX_MODELS];	// NULL terminated				//4
	struct qmodel_s	*models[MAX_MODELS];										//4
	const char	*sound_precache[MAX_SOUNDS];	// NULL terminated				//4
	const char	*lightstyles[MAX_LIGHTSTYLES];									//4
	int			num_edicts;														//4
	int			max_edicts;														//4
	edict_t		*edicts;			// can NOT be array indexed, because		//4
									// edict_t is variable sized, but can
									// be used to reference the world ent
	server_state_t	state;			// some actions are only valid during load	//1

	sizebuf_t	datagram;														//14
	byte		datagram_buf[MAX_DATAGRAM];										//1024

	sizebuf_t	reliable_datagram;	// copied to all clients at end of frame	//14
	byte		reliable_datagram_buf[MAX_DATAGRAM];							//1024

	sizebuf_t	*signon;														//4
	int			num_signon_buffers;												//4
	sizebuf_t	*signon_buffers[MAX_SIGNON_BUFFERS];							//4

	unsigned	protocol; //johnfitz											//4
	unsigned	protocolflags;													//4
} server_t;																		//2,272 Bytes (Surface level)


#define	NUM_PING_TIMES		16
#define	NUM_SPAWN_PARMS		16

//Be 8-bit
enum sendsignon_e
{
	PRESPAWN_DONE,
	uint8_t PRESPAWN_FLUSH=1,
	PRESPAWN_SIGNONBUFS,
	PRESPAWN_SIGNONMSG,
};

typedef struct client_s
{
	qboolean		active;				// false = client is free				//1
	qboolean		spawned;			// false = don't send datagrams			//1
	qboolean		dropasap;			// has been told to go to another level	//1
	enum sendsignon_e	sendsignon;			// only valid before spawned		//1
	int				signonidx;													//4

	double			last_message;		// reliable messages must be sent		//4
										// periodically

	struct qsocket_s *netconnection;	// communications handle				//4

	usercmd_t		cmd;				// movement 							//12
	vec3_t			wishdir;			// intended motion calced from cmd 		//6

	sizebuf_t		message;			// can be added to at any time, 		//14
										// copied and clear once per frame
	byte			msgbuf[MAX_MSGLEN];											//16000
	edict_t			*edict;				// EDICT_NUM(clientnum+1)				//4
	char			name[32];			// for printing to other people			//32
	int				colors;														//4

	float			ping_times[NUM_PING_TIMES];									//32
	int				num_pings;			// ping_times[num_pings%NUM_PING_TIMES]	//4

// spawn parms are carried from level to level
	float			spawn_parms[NUM_SPAWN_PARMS];								//2

// client known data for deltas
	int				old_frags;													//4
} client_t;																		//16130 Bytes (Surface level)


//=============================================================================

// edict->movetype values
#define	MOVETYPE_NONE			0		// never moves
#define	MOVETYPE_ANGLENOCLIP	1
#define	MOVETYPE_ANGLECLIP		2
#define	MOVETYPE_WALK			3		// gravity
#define	MOVETYPE_STEP			4		// gravity, special edge handling
#define	MOVETYPE_FLY			5
#define	MOVETYPE_TOSS			6		// gravity
#define	MOVETYPE_PUSH			7		// no clip to world, push and crush
#define	MOVETYPE_NOCLIP			8
#define	MOVETYPE_FLYMISSILE		9		// extra size to monsters
#define	MOVETYPE_BOUNCE			10
#define	MOVETYPE_GIB			11		// 2021 rerelease gibs

// edict->solid values
#define	SOLID_NOT				0		// no interaction with other objects
#define	SOLID_TRIGGER			1		// touch on edge, but not blocking
#define	SOLID_BBOX				2		// touch on edge, block
#define	SOLID_SLIDEBOX			3		// touch on edge, but not an onground
#define	SOLID_BSP				4		// bsp clip, touch on edge, block

// edict->deadflag values
#define	DEAD_NO					0
#define	DEAD_DYING				1
#define	DEAD_DEAD				2

#define	DAMAGE_NO				0
#define	DAMAGE_YES				1
#define	DAMAGE_AIM				2

// edict->flags
#define	FL_FLY					1
#define	FL_SWIM					2
//#define	FL_GLIMPSE				4
#define	FL_CONVEYOR				4
#define	FL_CLIENT				8
#define	FL_INWATER				16
#define	FL_MONSTER				32
#define	FL_GODMODE				64
#define	FL_NOTARGET				128
#define	FL_ITEM					256
#define	FL_ONGROUND				512
#define	FL_PARTIALGROUND		1024	// not all corners are valid
#define	FL_WATERJUMP			2048	// player jumping out of water
#define	FL_JUMPRELEASED			4096	// for jump debouncing

// entity effects

#define	EF_BRIGHTFIELD			1
#define	EF_MUZZLEFLASH 			2
#define	EF_BRIGHTLIGHT 			4
#define	EF_DIMLIGHT 			8

#define	SPAWNFLAG_NOT_EASY			256
#define	SPAWNFLAG_NOT_MEDIUM		512
#define	SPAWNFLAG_NOT_HARD			1024
#define	SPAWNFLAG_NOT_DEATHMATCH	2048

//============================================================================

extern	cvar_t	teamplay;
extern	cvar_t	skill;
extern	cvar_t	deathmatch;
extern	cvar_t	coop;
extern	cvar_t	fraglimit;
extern	cvar_t	timelimit;

extern	server_static_t	svs;				// persistant server info
extern	server_t		sv;					// local server

extern	client_t	*host_client;

extern	edict_t		*sv_player;

//===========================================================

void SV_Init (void);

void SV_StartParticle (vec3_t org, vec3_t dir, int color, int count);
void SV_StartSound (edict_t *entity, int channel, const char *sample, int volume,
    float attenuation);
void SV_LocalSound (client_t *client, const char *sample); // for 2021 rerelease

void SV_DropClient (qboolean crash);

void SV_SendClientMessages (void);
void SV_ClearDatagram (void);
void SV_ReserveSignonSpace (int numbytes);

int SV_ModelIndex (const char *name);

void SV_SetIdealPitch (void);

void SV_AddUpdates (void);

void SV_ClientThink (void);
void SV_AddClientToServer (struct qsocket_s	*ret);

void SV_ClientPrintf (const char *fmt, ...) FUNC_PRINTF(1,2);
void SV_BroadcastPrintf (const char *fmt, ...) FUNC_PRINTF(1,2);

void SV_Physics (void);

qboolean SV_CheckBottom (edict_t *ent);
qboolean SV_movestep (edict_t *ent, vec3_t move, qboolean relink);

void SV_WriteClientdataToMessage (edict_t *ent, sizebuf_t *msg);

void SV_MoveToGoal (void);

void SV_CheckForNewClients (void);
void SV_RunClients (void);
void SV_SaveSpawnparms (void);
void SV_SpawnServer (const char *server);

#endif	/* QUAKE_SERVER_H */
