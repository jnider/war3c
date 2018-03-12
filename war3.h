#ifndef _WAR3__H
#define _WAR3__H

#pragma pack(1)

#define BLIZZARD_ID_WARCRAFT_3	"PX3W" // W3XP

enum w3_msg_id
{
	W3GS_SEARCH_GAME = 0x2F,
	W3GS_GAME_INFO,
	W3GS_CREATE_GAME,
	W3GS_REFRESH_GAME,
	W3GS_END_GAME, // originally DECREATEGAME, which isn't a word
};

typedef struct w3gs_header
{
	unsigned char a;			// magic FF
	unsigned char msg_id; 	// see W3_MSG_ID
	unsigned short len;		// total message length (including this header)
} w3gs_header;

typedef struct w3gs_search_game // 0x2F
{
	w3gs_header header;
	char product[4];	// see BLIZZARD_ID_
	unsigned int version;	// version of app
	unsigned int unknown;	// 
} w3gs_search_game;

typedef struct w3gs_create_game // 0x31
{
	w3gs_header header;
	char product[4];	// see BLIZZARD_ID_
	unsigned int version;	// version of app
	unsigned int counter;	// index of game started by host
} w3gs_create_game;

typedef struct w3gs_refresh_game // 0x32
{
	w3gs_header header;
	unsigned int counter;	// index of game started by host
	unsigned int players;	// how many players are currently in the game
	unsigned int slots;		// how many slots in the game
} w3gs_refresh_game;

typedef struct w3gs_end_game // 0x33
{
	w3gs_header header;
	unsigned int counter;
} w3gs_end_game;

#endif // _WAR3__H

