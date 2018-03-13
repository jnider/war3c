#ifndef _WAR3__H
#define _WAR3__H

#pragma pack(1)

#define BLIZZARD_ID_WARCRAFT_3	"PX3W" // W3XP

enum w3_msg_id
{
	W3GS_SLOT_INFO_JOIN = 0x04,
	W3GS_REQ_JOIN = 0x1E,
	W3GS_SEARCH_GAME = 0x2F,
	W3GS_GAME_INFO,
	W3GS_CREATE_GAME,
	W3GS_REFRESH_GAME,
	W3GS_END_GAME, // originally DECREATEGAME, which isn't a word
};

typedef struct w3gs_header
{
	unsigned char a;			// magic F7
	unsigned char msg_id; 	// see W3_MSG_ID
	unsigned short len;		// total message length (including this header)
} w3gs_header;

////////// TCP ////////////
typedef struct w3gs_req_join // 0x1E
{
	w3gs_header header;
	unsigned int game_id;	// index of game started by host
	unsigned int key;
	char unknown;
	unsigned short port;
	unsigned int peer_key;
	char name[8];
	unsigned short unknown2;
	unsigned short unknown3;
	unsigned short int_port;
	unsigned int int_ip;
	unsigned int unknown4;
	unsigned int unknown5;
	//char unknown6;
} w3gs_req_join;

////////// UDP ////////////
typedef struct w3gs_search_game // 0x2F
{
	w3gs_header header;
	char product[4];	// see BLIZZARD_ID_
	unsigned int version;	// version of app
	unsigned int unknown;	// 
} w3gs_search_game;

typedef struct w3gs_game_info // 0x30
{
	w3gs_header header;
	char product[4];	// see BLIZZARD_ID_
	unsigned int version;	// version of app
	unsigned int game_id;	// index of game started by host
	unsigned int key;			// need this key to join the game
	char name[8];
} w3gs_game_info;

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

