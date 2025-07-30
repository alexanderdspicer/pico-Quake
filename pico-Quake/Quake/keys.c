/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
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
#include "arch_def.h"

#include "keys.h"
#include "usb.h"

/* key up events are sent even if in console mode */

#define		HISTORY_FILE_NAME "history.txt"

char		key_lines[CMDLINES][MAXCMDLINE];

int			key_linepos;
int			key_insert;	//johnfitz -- insert key toggle (for editing)
double		key_blinktime; //johnfitz -- fudge cursor blinking to make it easier to spot in certain cases

int			edit_line = 0;
int			history_line = 0;

keydest_t	key_dest;

char		*keybindings[MAX_KEYS];
//qboolean	consolekeys[MAX_KEYS];	// if true, can't be rebound while in console
//qboolean	menubound[MAX_KEYS];	// if true, can't be rebound while in menu
//qboolean	keydown[MAX_KEYS];

#define CONSOLE_KEY 0b10000000

typedef struct
{
    uint8_t		modifiers;
    uint8_t		keynum;
    const char	*name;
} keyname_t;

//https://usb.org/sites/default/files/hut1_6.pdf, Page 90
keyname_t keynames[] =
{
    {0x00, CONSOLE_KEY | 0x2b, "TAB"},
    {0x00, CONSOLE_KEY | 0x28, "ENTER"},
    {0x00, CONSOLE_KEY | 0x29, "ESCAPE"},
    {0x00, CONSOLE_KEY | 0x2c, "SPACE"},
    {0x00, CONSOLE_KEY | 0x2a, "BACKSPACE"},
    {0x00, CONSOLE_KEY | 0x52, "UPARROW"},
    {0x00, CONSOLE_KEY | 0x51, "DOWNARROW"},
    {0x00, CONSOLE_KEY | 0x50, "LEFTARROW"},
    {0x00, CONSOLE_KEY | 0x4f, "RIGHTARROW"},
    {0x00, CONSOLE_KEY | 0x49, "INS"},
    {0x00, CONSOLE_KEY | 0x4c, "DEL"},
    {0x00, CONSOLE_KEY | 0x4e, "PGDN"},
    {0x00, CONSOLE_KEY | 0x4b, "PGUP"},
    {0x00, CONSOLE_KEY | 0x4a, "HOME"},
    {0x00, CONSOLE_KEY | 0x4d, "END"},

    {0x00, CONSOLE_KEY | 0x48, "PAUSE"},

    {0x00, CONSOLE_KEY | 0x33, "SEMICOLON"},		// because a raw semicolon seperates commands

    {0x00, 		CONSOLE_KEY | 0x35, "BACKQUOTE"},	// because a raw backquote may toggle the console
    {ANY_SHIFT, CONSOLE_KEY | 0x35, "TILDE"},		// because a raw tilde may toggle the console

    {ANY_ALT, 		CONSOLE_KEY | 0x00, "ALT"},
    {ANY_CONTROL, 	CONSOLE_KEY | 0x00, "CTRL"},
    {ANY_SHIFT, 	CONSOLE_KEY | 0x00, "SHIFT"},

    {0x00, CONSOLE_KEY | 0x53, "KP_NUMLOCK"},
    {0x00, CONSOLE_KEY | 0x54, "KP_SLASH"},
    {0x00, CONSOLE_KEY | 0x55, "KP_STAR"},
    {0x00, CONSOLE_KEY | 0x56, "KP_MINUS"},
    {0x00, CONSOLE_KEY | 0x5f, "KP_HOME"},
    {0x00, CONSOLE_KEY | 0x60, "KP_UPARROW"},
    {0x00, CONSOLE_KEY | 0x61, "KP_PGUP"},
    {0x00, CONSOLE_KEY | 0x57, "KP_PLUS"},
    {0x00, CONSOLE_KEY | 0x5c, "KP_LEFTARROW"},
    {0x00, CONSOLE_KEY | 0x5d, "KP_5"},
    {0x00, CONSOLE_KEY | 0x5e, "KP_RIGHTARROW"},
    {0x00, CONSOLE_KEY | 0x59, "KP_END"},
    {0x00, CONSOLE_KEY | 0x5a, "KP_DOWNARROW"},
    {0x00, CONSOLE_KEY | 0x5b, "KP_PGDN"},
    {0x00, CONSOLE_KEY | 0x58, "KP_ENTER"},
    {0x00, CONSOLE_KEY | 0x62, "KP_INS"},
    {0x00, CONSOLE_KEY | 0x63, "KP_DEL"},

    //{"F1", K_F1},
    //{"F2", K_F2},
    //{"F3", K_F3},
    //{"F4", K_F4},
    //{"F5", K_F5},
    //{"F6", K_F6},
    //{"F7", K_F7},
    //{"F8", K_F8},
    //{"F9", K_F9},
    //{"F10", K_F10},
    //{"F11", K_F11},
    //{"F12", K_F12},

    {NULL,				0}
};

/*
==============================================================================

            LINE TYPING INTO THE CONSOLE

==============================================================================
*/

/*
====================
Key_Console -- johnfitz -- heavy revision

Interactive line editing and console scrollback
====================
*/
extern	char    *con_text, key_tabpartial[MAXCMDLINE];
extern	int     con_current, con_linewidth, con_vislines;

void Key_Console (int key)
{
    static	char current[MAXCMDLINE] = "";
    int	    history_line_last;
    size_t	len;
    char    *workline = key_lines[edit_line];

    switch (key)
    {
    case K_ENTER:
    case K_KP_ENTER:
        key_tabpartial[0] = 0;
        Cbuf_AddText (workline + 1);	// skip the prompt
        Cbuf_AddText ("\n");
        Con_Printf ("%s\n", workline);

        // If the last two lines are identical, skip storing this line in history 
        // by not incrementing edit_line
        if (strcmp(workline, key_lines[(edit_line - 1) & (CMDLINES - 1)]))
            edit_line = (edit_line + 1) & (CMDLINES - 1);

        history_line = edit_line;
        key_lines[edit_line][0] = ']';
        key_lines[edit_line][1] = 0; //johnfitz -- otherwise old history items show up in the new edit line
        key_linepos = 1;
        if (cls.state == ca_disconnected)
            SCR_UpdateScreen (); // force an update, because the command may take some time
        return;

    case K_TAB:
        Con_TabComplete ();
        return;

    case K_BACKSPACE:
        key_tabpartial[0] = 0;
        if (key_linepos > 1)
        {
            workline += key_linepos - 1;
            if (workline[1])
            {
                len = strlen(workline);
                memmove (workline, workline + 1, len);
            }
            else	*workline = 0;
            key_linepos--;
        }
        return;

    case K_DEL:
        key_tabpartial[0] = 0;
        workline += key_linepos;
        if (*workline)
        {
            if (workline[1])
            {
                len = strlen(workline);
                memmove (workline, workline + 1, len);
            }
            else	*workline = 0;
        }
        return;

    case K_HOME:
        if (keydown[K_CTRL])
        {
            //skip initial empty lines
            int i, x;
            char *line;

            for (i = con_current - con_totallines + 1; i <= con_current; i++)
            {
                line = con_text + (i % con_totallines) * con_linewidth;
                for (x = 0; x < con_linewidth; x++)
                {
                    if (line[x] != ' ')
                        break;
                }
                if (x != con_linewidth)
                    break;
            }
            con_backscroll = CLAMP(0, con_current-i%con_totallines-2, con_totallines-(glheight>>3)-1);
        }
        else	key_linepos = 1;
        return;

    case K_END:
        if (keydown[K_CTRL])
            con_backscroll = 0;
        else	key_linepos = strlen(workline);
        return;

    case K_PGUP:
    //case K_MWHEELUP:
        con_backscroll += keydown[K_CTRL] ? ((con_vislines>>3) - 4) : 2;
        if (con_backscroll > con_totallines - (vid.height>>3) - 1)
            con_backscroll = con_totallines - (vid.height>>3) - 1;
        return;

    case K_PGDN:
    //case K_MWHEELDOWN:
        con_backscroll -= keydown[K_CTRL] ? ((con_vislines>>3) - 4) : 2;
        if (con_backscroll < 0)
            con_backscroll = 0;
        return;

    case K_LEFTARROW:
        if (key_linepos > 1)
        {
            key_linepos--;
            key_blinktime = realtime;
        }
        return;

    case K_RIGHTARROW:
        len = strlen(workline);
        if ((int)len == key_linepos)
        {
            len = strlen(key_lines[(edit_line + (CMDLINES - 1)) & (CMDLINES - 1)]);
            if ((int)len <= key_linepos)
                return; // no character to get
            workline += key_linepos;
            *workline = key_lines[(edit_line + (CMDLINES - 1)) & (CMDLINES - 1)][key_linepos];
            workline[1] = 0;
            key_linepos++;
        }
        else
        {
            key_linepos++;
            key_blinktime = realtime;
        }
        return;

    case K_UPARROW:
        if (history_line == edit_line)
            Q_strcpy(current, workline);

        history_line_last = history_line;
        do
        {
            history_line = (history_line - 1) & (CMDLINES - 1);
        } while (history_line != edit_line && !key_lines[history_line][1]);

        if (history_line == edit_line)
        {
            history_line = history_line_last;
            return;
        }

        key_tabpartial[0] = 0;
        len = strlen(key_lines[history_line]);
        memmove(workline, key_lines[history_line], len+1);
        key_linepos = (int)len;
        return;

    case K_DOWNARROW:
        if (history_line == edit_line)
            return;

        key_tabpartial[0] = 0;

        do
        {
            history_line = (history_line + 1) & (CMDLINES - 1);
        } while (history_line != edit_line && !key_lines[history_line][1]);

        if (history_line == edit_line)
        {
            len = strlen(current);
            memcpy(workline, current, len+1);
        }
        else
        {
            len = strlen(key_lines[history_line]);
            memmove(workline, key_lines[history_line], len+1);
        }
        key_linepos = (int)len;
        return;

    case K_INS:
        key_insert ^= 1;
        return;

    case 'v':
    case 'V':
        //Pasting removed, no host OS

    case 'c':
    case 'C':
        if (keydown[K_CTRL]) {		/* Ctrl+C: abort the line -- S.A */
            Con_Printf ("%s\n", workline);
            workline[0] = ']';
            workline[1] = 0;
            key_linepos = 1;
            history_line= edit_line;
            return;
        }
        break;
    }
}

void Char_Console (int key)
{
    size_t		len;
    char *workline = key_lines[edit_line];

    if (key_linepos < MAXCMDLINE-1)
    {
        qboolean endpos = !workline[key_linepos];

        key_tabpartial[0] = 0; //johnfitz
        // if inserting, move the text to the right
        if (key_insert && !endpos)
        {
            workline[MAXCMDLINE - 2] = 0;
            workline += key_linepos;
            len = strlen(workline) + 1;
            memmove (workline + 1, workline, len);
            *workline = key;
        }
        else
        {
            workline += key_linepos;
            *workline = key;
            // null terminate if at the end
            if (endpos)
                workline[1] = 0;
        }
        key_linepos++;
    }
}

//============================================================================

qboolean	chat_team = false;
static char	chat_buffer[MAXCMDLINE];
static int	chat_bufferlen = 0;

const char *Key_GetChatBuffer (void)
{
    return chat_buffer;
}

int Key_GetChatMsgLen (void)
{
    return chat_bufferlen;
}

void Key_EndChat (void)
{
    key_dest = key_game;
    chat_bufferlen = 0;
    chat_buffer[0] = 0;
}

void Key_Message (uint16_t key)
{
    switch (key)
    {
    case K_ENTER:
    case K_KP_ENTER:
        if (chat_team)
            Cbuf_AddText ("say_team \"");
        else
            Cbuf_AddText ("say \"");
        Cbuf_AddText(chat_buffer);
        Cbuf_AddText("\"\n");

        Key_EndChat ();
        return;

    case K_ESCAPE:
        Key_EndChat ();
        return;

    case K_BACKSPACE:
        if (chat_bufferlen)
            chat_buffer[--chat_bufferlen] = 0;
        return;
    }
}

void Char_Message (int key)
{
    if (chat_bufferlen == sizeof(chat_buffer) - 1)
        return; // all full

    chat_buffer[chat_bufferlen++] = key;
    chat_buffer[chat_bufferlen] = 0;
}

//============================================================================

//Could use the actual chars for the first value, but the hex notation
//I believe better shows the seperation between them and why this problem
//has no (that I have yet to find) easy, elegant solution.

//Two way list from hell
static uint8_t[32][3] twlfh = {
    {0x21, 0x1e, ANY_SHIFT}, //!
    {0x22, 0x34, ANY_SHIFT}, //"
    {0x23, 0x20, ANY_SHIFT}, //#
    {0x24, 0x21, ANY_SHIFT}, //$
    {0x25, 0x22, ANY_SHIFT}, //%
    {0x26, 0x24, ANY_SHIFT}, //&
    {0x27, 0x34, 0x00}, 	 //'
    {0x28, 0x26, ANY_SHIFT}, //(
    {0x29, 0x27, ANY_SHIFT}, //)
    {0x2a, 0x25, ANY_SHIFT}, //*
    {0x2b, 0x2e, ANY_SHIFT}, //+
    {0x2c, 0x36, 0x00}, 	 //,
    {0x2d, 0x2d, 0x00}, 	 //-
    {0x2e, 0x34, 0x00}, 	 //.
    {0x2f, 0x34, 0x00}, 	 ///

    {0x3a, 0x33, 0x00}, 	 //;
    {0x3b, 0x33, ANY_SHIFT}, //:
    {0x3c, 0x36, ANY_SHIFT}, //<
    {0x3d, 0x2e, 0x00},      //=
    {0x3e, 0x37, ANY_SHIFT}, //>
    {0x3f, 0x38, 0x00}, 	 //?
    {0x40, 0x1f, ANY_SHIFT}, //@

    {0x5b, 0x2f, 0x00}, 	 //[
    {0x5c, 0x31, 0x00}, 	 //'\'
    {0x5d, 0x30, 0x00}, 	 //]
    {0x5e, 0x23, ANY_SHIFT}, //^
    {0x5f, 0x2d, ANY_SHIFT}, //_
    {0x60, 0x35, 0x00}, 	 //`

    {0x7b, 0x2f, ANY_SHIFT}, //{
    {0x7c, 0x31, ANY_SHIFT}, //|
    {0x7d, 0x30, ANY_SHIFT}, //}
    {0x7e, 0x35, ANY_SHIFT}, //~
}

//The 16 bit 'input' is two fold, firstly:
//	the first 8 bits are the actual keycode, while the other 8 bits represent the "special" keys
//	such as control, alt, shift, 'gui'.
//secondly:
//	due to the limited "MAX_KEYS" macro, 'input' may never naturally (barring specialized keyboards
//	or faulty cables) be -1, so -1 is an appropriate error state (regardless of extenuating circumstances)
//	and is grounds to prematurely end the function/coroutine
int16_t conversion(int8_t[2] input, char* ret_val) {
    if(*((int16_t*)input) == -1) {
        return -1;
    }
    int16_t return_val = 0;
    switch (*ret_val) {
        case -1:	//Keyboard to ascii
            if(input[1] > 0x03 && input[1] < 0x1e) {
                *ret_val = 'A' + (input[1] - 0x04);
                if(input[0] & ANY_SHIFT) {
                    *ret_val |= 0b00100000;
                }
            } else if(input[1] > 0x1d && input[1] < 0x28) {
                if(input[1] == 0x27) {
                    *ret_val = '0'
                } else {
                    *ret_val = '1' + (input[1] - 0x1e);
                }
            } else {
                for(uint8_t i = 0; i<32; i++) {
                    if(input[1] == twlfh[i][1] && (input[0] & ANY_SHIFT)) {
                        *ret_val = twlfh[i][0];
                    }
                }
            }
            break;
        default:	//Ascii to keyboard -- (Narrowing conversion)
            uint8_t offset = 0;
            if(*ret_val < 0x30 && *ret_val > 0x20) {
                offset = *ret_val - 0x21;
                return_val = twlfh[offset][1] | (twlfh[offset][2] << 8);
            } else if (*ret_val < 0x31 && *ret_val > 0x3a) {
                if(*ret_val == 0x30) {
                    return_val = 0x0027
                }
                return_val = *ret_val - (0x31 - 0x1e);
            } else if (*ret_val < 0x41 && *ret_val > 0x39) {
                offset = (*ret_val - 0x3a) + 15;
                return_val = twlfh[offset][1] | (twlfh[offset][2] << 8);
            } else if (*ret_val < 0x5b && *ret_val > 0x40) {
                return_val = (*ret_val - (0x41 - 0x03)) | (ANY_SHIFT << 8);
            } else if (*ret_val < 0x61 && *ret_val > 0x5a) {
                offset = (*ret_val - 0x5b) + 22;
                return_val = twlfh[offset][1] | (twlfh[offset][2] << 8);
            } else if (*ret_val < 0x7b && *ret_val > 0x60) {
                return_val = (*ret_val - (0x61 - 0x03));
            } else if (*ret_val < 0x7f && *ret_val > 0x7a) {
                offset = (*ret_val - 0x7b) + 28;
                return_val = twlfh[offset][1] | (twlfh[offset][2] << 8);
            } else {
                //Uh oh
                return -1;
            }
            return return_val | CONSOLE_KEY;
            break;
    }
    return 0;
}

/*
===================
Key_StringToKeynum

Returns a key number to be used to index keybindings[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.
===================
*/
int16_t Key_StringToKeynum (const char *str)
{
    keyname_t	*kn;

    if (!str || !str[0])
        return -1;
    char ret_val = 0;
    if (!str[1]) {
        //We dont care about the 'case' of the keypress, so bitmask so just the keycode is left
        return conversion(0, &ret_val) & 0x00ff;
    }

    //potential need of recast '&ret_val'(?)
    if(sscanf_s(str, "0x%02X", &ret_val));
        return ret_val; //Expanding conversion? (its been a while)

    for (kn=keynames; kn->name ; kn++)
    {
        if (!q_strcasecmp(str,kn->name))
            return *((int16_t*)kn);
    }
    return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, or a K_* name) for the
given keynum.
FIXME: handle quote special (general escape sequence?)
===================
*/
const char *Key_KeynumToString (int16_t keynum)
{
    static	char	tinystr[2];
    keyname_t	*kn;

    if (keynum == -1)
        return "<KEY NF>";
    char ret_val = -1;
    if(conversion(keynum, &ret_val) != -1)
        return ret_val; //Char to Const Char conversion I know will be a problem

    for (kn = keynames; kn->name; kn++)
    {
        if (keynum == *((int16_t*)kn))
            return kn->name;
    }

    return "<NONKEY>";
}


/*
===================
Key_SetBinding
===================
*/
void Key_SetBinding (int16_t keynum, const char *binding)
{
    if (keynum == -1)
        return;

// free old bindings
    if (keybindings[keynum])
    {
        Z_Free (keybindings[keynum]);
        keybindings[keynum] = NULL;
    }

// allocate memory for new binding
    if (binding)
        keybindings[keynum] = Z_Strdup(binding);
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f (void)
{
    int16_t	b;

    if (Cmd_Argc() != 2)
    {
        Con_Printf ("unbind <key> : remove commands from a key\n");
        return;
    }

    b = Key_StringToKeynum (Cmd_Argv(1));
    if (b == -1)
    {
        Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
        return;
    }

    Key_SetBinding (b, NULL);
}

void Key_Unbindall_f (void)
{
    int	i;

    for (i = 0; i < MAX_KEYS; i++)
    {
        if (keybindings[i])
            Key_SetBinding (i, NULL);
    }
}

/*
============
Key_Bindlist_f -- johnfitz
============
*/
void Key_Bindlist_f (void)
{
    int	i, count;

    count = 0;
    for (i = 0; i < MAX_KEYS; i++)
    {
        if (keybindings[i] && *keybindings[i])
        {
            Con_SafePrintf ("   %s \"%s\"\n", Key_KeynumToString(i), keybindings[i]);
            count++;
        }
    }
    Con_SafePrintf ("%i bindings\n", count);
}

/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f (void)
{
    int	i, c;
    int16_t b;
    char	cmd[1024];

    c = Cmd_Argc();

    if (c != 2 && c != 3)
    {
        Con_Printf ("bind <key> [command] : attach a command to a key\n");
        return;
    }
    b = Key_StringToKeynum (Cmd_Argv(1));
    if (b == -1)
    {
        Con_Printf ("\"%s\" isn't a valid key\n", Cmd_Argv(1));
        return;
    }

    if (c == 2)
    {
        if (keybindings[b])
            Con_Printf ("\"%s\" = \"%s\"\n", Cmd_Argv(1), keybindings[b] );
        else
            Con_Printf ("\"%s\" is not bound\n", Cmd_Argv(1) );
        return;
    }

// copy the rest of the command line
    cmd[0] = 0;
    for (i = 2; i < c; i++)
    {
        q_strlcat (cmd, Cmd_Argv(i), sizeof(cmd));
        if (i != (c-1))
            q_strlcat (cmd, " ", sizeof(cmd));
    }

    Key_SetBinding (b, cmd);
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings (FILE *f)
{
    int	i;

    // unbindall before loading stored bindings:
    if (cfg_unbindall.value)
        fprintf (f, "unbindall\n");
    for (i = 0; i < MAX_KEYS; i++)
    {
        if (keybindings[i] && *keybindings[i])
            fprintf (f, "bind \"%s\" \"%s\"\n", Key_KeynumToString(i), keybindings[i]);
    }
}


void History_Init (void)
{
    int i, c;
    FILE *hf;

    for (i = 0; i < CMDLINES; i++)
    {
        key_lines[i][0] = ']';
        key_lines[i][1] = 0;
    }
    key_linepos = 1;

    hf = fopen(va("%s/%s", host_parms->userdir, HISTORY_FILE_NAME), "rt");
    if (hf != NULL)
    {
        do
        {
            i = 1;
            do
            {
                c = fgetc(hf);
                key_lines[edit_line][i++] = c;
            } while (c != '\r' && c != '\n' && c != EOF && i < MAXCMDLINE);
            key_lines[edit_line][i - 1] = 0;
            edit_line = (edit_line + 1) & (CMDLINES - 1);
            /* for people using a windows-generated history file on unix: */
            if (c == '\r' || c == '\n')
            {
                do
                    c = fgetc(hf);
                while (c == '\r' || c == '\n');
                if (c != EOF)
                    ungetc(c, hf);
                else	c = 0; /* loop once more, otherwise last line is lost */
            }
        } while (c != EOF && edit_line < CMDLINES);
        fclose(hf);

        history_line = edit_line = (edit_line - 1) & (CMDLINES - 1);
        key_lines[edit_line][0] = ']';
        key_lines[edit_line][1] = 0;
    }
}

void History_Shutdown (void)
{
    int i;
    FILE *hf;

    hf = fopen(va("%s/%s", host_parms->userdir, HISTORY_FILE_NAME), "wt");
    if (hf != NULL)
    {
        i = edit_line;
        do
        {
            i = (i + 1) & (CMDLINES - 1);
        } while (i != edit_line && !key_lines[i][1]);

        while (i != edit_line && key_lines[i][1])
        {
            fprintf(hf, "%s\n", key_lines[i] + 1);
            i = (i + 1) & (CMDLINES - 1);
        }
        fclose(hf);
    }
}

/*
===================
Key_Init
===================
*/
void Key_Init (void)
{
    int	i;

    History_Init ();

    key_blinktime = realtime; //johnfitz

//
// initialize menubound[] //picoQuake -- pretty sure this is a 'hoax', and not needed
//
    /*menubound[K_ESCAPE] = true;
    for (i = 0; i < 12; i++)
        menubound[K_F1+i] = true;*/

//
// register our functions
//
    Cmd_AddCommand ("bindlist",Key_Bindlist_f); //johnfitz
    Cmd_AddCommand ("bind",Key_Bind_f);
    Cmd_AddCommand ("unbind",Key_Unbind_f);
    Cmd_AddCommand ("unbindall",Key_Unbindall_f);
}

static struct {
    qboolean active;
    int lastkey;
    int lastchar;
} key_inputgrab = { false, -1, -1 };

/*
===================
Key_BeginInputGrab
===================
*/
void Key_BeginInputGrab (void)
{
    Key_ClearStates ();

    key_inputgrab.active = true;
    key_inputgrab.lastkey = -1;
    key_inputgrab.lastchar = -1;

    IN_UpdateInputMode ();
}

/*
===================
Key_EndInputGrab
===================
*/
void Key_EndInputGrab (void)
{
    Key_ClearStates ();

    key_inputgrab.active = false;

    IN_UpdateInputMode ();
}

/*
===================
Key_GetGrabbedInput
===================
*/
void Key_GetGrabbedInput (int *lastkey, int *lastchar)
{
    if (lastkey)
        *lastkey = key_inputgrab.lastkey;
    if (lastchar)
        *lastchar = key_inputgrab.lastchar;
}

/*
===================
Key_Event

Called by the system between frames for both key up and key down events
Should NOT be called during an interrupt!
===================
*/
void Key_Event (int key, qboolean down)
{
    char	*kb;
    char	cmd[1024];

    if (key == -1 || key >= MAX_KEYS)
        return;

    // handle fullscreen toggle
    // picoQuake -- always full screen

// handle autorepeats and stray key up events
    if (down)
    {
        if (keydown[key])
        {
            if (key_dest == key_game && !con_forcedup)
                return; // ignore autorepeats in game mode
        }
        else if (key >= 200 && !keybindings[key])
            Con_Printf ("%s is unbound, hit F4 to set.\n", Key_KeynumToString(key));
    }
    else if (!keydown[key])
        return; // ignore stray key up events

    keydown[key] = down;

    if (key_inputgrab.active)
    {
        if (down)
        {
            key_inputgrab.lastkey = key;
            if (keycode > 0)
                key_inputgrab.lastchar = keycode;
        }
        return;
    }

// handle escape specially, so the user can never unbind it
    if (key == K_ESCAPE)
    {
        if (!down)
            return;

        if (keydown[K_SHIFT])
        {
            Con_ToggleConsole_f();
            return;
        }

        switch (key_dest)
        {
        case key_message:
            Key_Message (key);
            break;
        case key_menu:
            M_Keydown (key);
            break;
        case key_game:
        case key_console:
            M_ToggleMenu_f ();
            break;
        default:
            Sys_Error ("Bad key_dest");
        }

        return;
    }

// key up events only generate commands if the game key binding is
// a button command (leading + sign).  These will occur even in console mode,
// to keep the character from continuing an action started before a console
// switch.  Button commands include the kenum as a parameter, so multiple
// downs can be matched with ups
    if (!down)
    {
        kb = keybindings[key];
        if (kb && kb[0] == '+')
        {
            sprintf (cmd, "-%s %i\n", kb+1, key);
            Cbuf_AddText (cmd);
        }
        return;
    }

// during demo playback, most keys bring up the main menu
    if (cls.demoplayback && down && consolekeys[key] && key_dest == key_game && key != K_TAB)
    {
        M_ToggleMenu_f ();
        return;
    }

// if not a consolekey, send to the interpreter no matter what mode is
    if ((key_dest == key_menu /*&& menubound[key]*/) ||
        (key_dest == key_console && !consolekeys[key]) ||
        (key_dest == key_game && (!con_forcedup || !consolekeys[key])))
    {
        kb = keybindings[key];
        if (kb)
        {
            if (kb[0] == '+')
            {	// button commands add keynum as a parm
                sprintf (cmd, "%s %i\n", kb, key);
                Cbuf_AddText (cmd);
            }
            else
            {
                Cbuf_AddText (kb);
                Cbuf_AddText ("\n");
            }
        }
        return;
    }

    if (!down)
        return;		// other systems only care about key down events

    switch (key_dest)
    {
    case key_message:
        Key_Message (key);
        break;
    case key_menu:
        M_Keydown (key);
        break;

    case key_game:
    case key_console:
        Key_Console (key);
        break;
    default:
        Sys_Error ("Bad key_dest");
    }
}

/*
===================
Char_Event

Called by the backend when the user has input a character.
===================
*/
void Char_Event (int key)
{
    if (key < 32 || key > 126)
        return;

    if (keydown[K_CTRL])
        return;

    if (key_inputgrab.active)
    {
        key_inputgrab.lastchar = key;
        return;
    }

    switch (key_dest)
    {
    case key_message:
        Char_Message (key);
        break;
    case key_menu:
        M_Charinput (key);
        break;
    case key_game:
        if (!con_forcedup)
            break;
        /* fallthrough */
    case key_console:
        Char_Console (key);
        break;
    default:
        break;
    }
}

/*
===================
Key_TextEntry
===================
*/
qboolean Key_TextEntry (void)
{
    if (key_inputgrab.active)
    {
        // This path is used for simple single-letter inputs (y/n prompts) that also
        // accept controller input, so we don't want an onscreen keyboard for this case.
        return false;
    }

    switch (key_dest)
    {
    case key_message:
        return true;
    case key_menu:
        return M_TextEntry();
    case key_game:
        // Don't return true even during con_forcedup, because that happens while starting a
        // game and we don't to trigger text input (and the onscreen keyboard on some devices)
        // during this.
        return false;
    case key_console:
        return true;
    default:
        return false;
    }
}

/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates (void)
{
    int	i;

    for (i = 0; i < MAX_KEYS; i++)
    {
        if (keydown[i])
            Key_Event (i, false);
    }
}

/*
===================
Key_UpdateForDest
===================
*/
void Key_UpdateForDest (void)
{
    static qboolean forced = false;

    if (cls.state == ca_dedicated)
        return;

    switch (key_dest)
    {
    case key_console:
        if (forced && cls.state == ca_connected)
        {
            forced = false;
            IN_Activate();
            key_dest = key_game;
        }
        break;
    case key_game:
        if (cls.state != ca_connected)
        {
            forced = true;
            IN_Deactivate(modestate == MS_WINDOWED);
            key_dest = key_console;
            break;
        }
    /* fallthrough */
    default:
        forced = false;
        break;
    }
}

