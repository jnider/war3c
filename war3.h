#ifndef _WAR3__H
#define _WAR3__H

#pragma pack(1)

#define BLIZZARD_ID_WARCRAFT_3_XP	"PX3W" // W3XP
#define BLIZZARD_ID_WARCRAFT_3	"3RAW"

enum w3_msg_id
{
	W3GS_PING = 0x01,
	W3GS_SLOT_INFO_JOIN = 0x04,
	W3GS_REJECT_JOIN,
	W3GS_PLAYER_INFO,
	W3GS_PLAYER_LEFT,
	W3GS_PLAYER_LOADED,
	W3GS_SLOT_INFO,
	W3GS_COUNTDOWN_START,
	W3GS_COUNTDOWN_END,
	W3GS_INCOMING_ACTION,
	W3GS_CHAT_FROM_HOST = 0x0F,
	W3GS_UNKNOWN2 = 0x11,
	W3GS_KICKED = 0x1C,
	W3GS_REQ_JOIN = 0x1E,
   W3GS_GAME_LOADED = 0x23,
	W3GS_OUTGOING_ACTION = 0x26,
	W3GS_KEEPALIVE, 				// 0x27
	W3GS_PLAYER_UPDATE, 			// 0x28 - original: CHAT_TO_HOST
	W3GS_SEARCH_GAME = 0x2F,
	W3GS_GAME_INFO,
	W3GS_CREATE_GAME,
	W3GS_REFRESH_GAME,
	W3GS_END_GAME, // originally DECREATEGAME, which isn't a word
	W3GS_MAP_CHECK = 0x3D,
	W3GS_UNKNOWN1 = 0x40,
	W3GS_MAP_SIZE = 0x42,
	W3GS_PONG = 0x46,
};

enum
{
	REASON_LEFT_DISCONNECT=0x01,
	REASON_LEFT_LOST=0x07,
	REASON_LEFT_LOST_BUILDINGS,
	REASON_LEFT_WON,
	REASON_LEFT_DRAW,
	REASON_LEFT_OBSERVER,
	REASON_LEFT_LOBBY = 0x0D,
};

enum
{
	PLAYER_TYPE_HUMAN,
	PLAYER_TYPE_COMPUTER
};

enum
{
	SLOT_STATUS_OPEN,
	SLOT_STATUS_CLOSED,
	SLOT_STATUS_OCCUPIED
};

enum
{
	RACE_HUMAN_SHIFT,
	RACE_ORC_SHIFT,
	RACE_ELF_SHIFT,
	RACE_UNDEAD_SHIFT,
	RACE_FIXED_SHIFT,
	RACE_RANDOM_SHIFT,
};

enum
{
	COMPUTER_TYPE_EASY,
	COMPUTER_TYPE_NORMAL,
	COMPUTER_TYPE_HARD
};

typedef struct w3gs_header
{
	unsigned char a;			// magic F7
	unsigned char msg_id; 	// see W3_MSG_ID
	unsigned short len;		// total message length (including this header)
} w3gs_header;

typedef struct w3gs_slot_info
{
	unsigned char player;
	unsigned char dl_status;
	unsigned char status;
	unsigned char type; // see PLAYER_TYPE_
	unsigned char team;
	unsigned char color;
	unsigned char race;
	unsigned char computer_type;
	unsigned char handicap;
} w3gs_slot_info;

typedef struct w3gs_slot_info_join_1 // 0x04 and 0x09
{
	w3gs_header header;
	unsigned short length;
	unsigned char num_slots;
	w3gs_slot_info slot[0];
} w3gs_slot_info_join_1;

typedef struct w3gs_slot_info_join_2 // 0x04
{
	unsigned int timestamp;
	unsigned short u1;
	char player; // your player id for this game
} w3gs_slot_info_join_2;

typedef struct w3gs_player_info_1 // 0x06
{
	w3gs_header header;
	unsigned int counter;
	unsigned char player;
	char name[0];
} w3gs_player_info_1;
// another one of these damn variable length strings in the middle of a packet!
typedef struct w3gs_player_info_2 // 0x06
{
	unsigned short unknown;
	unsigned short af_inet;
	unsigned short port;
	unsigned int ext_addr;
	unsigned int unknown2;
	unsigned int unknown3;
	unsigned short af_inet2;
	unsigned short int_port;
	unsigned int int_addr;
	unsigned int unknown4;
	unsigned int unknown5;
} w3gs_player_info_2;

typedef struct w3gs_player_left // 0x07
{
	w3gs_header header;
	unsigned char player;
	unsigned int reason; // see REASON_LEFT_
} w3gs_player_left;

typedef struct w3gs_player_loaded // 0x08
{
	w3gs_header header;
	unsigned char player;
} w3gs_player_loaded;

typedef struct w3gs_incoming_action // 0x0C
{
	w3gs_header header;
	unsigned char player;
	unsigned short len;
	char data[0];
} w3gs_incoming_action;

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
} w3gs_req_join;

typedef struct w3gs_keepalive // 0x27
{
	w3gs_header header;
	char u1;
	unsigned int val;
} w3gs_keepalive;

typedef struct w3gs_player_update // 0x28
{
	w3gs_header header;
	unsigned char f1;
	unsigned char f2;
	unsigned char player;
	unsigned char attr;
	unsigned char val;
} w3gs_player_update;

typedef struct w3gs_search_game // 0x2F
{
	w3gs_header header;
	char product[4];	// see BLIZZARD_ID_
	unsigned int version;	// version of app
	unsigned int unknown;	// 
} w3gs_search_game;

typedef struct w3gs_game_info_1 // 0x30
{
	w3gs_header header;
	char product[4];	// see BLIZZARD_ID_
	unsigned int version;	// version of app
	unsigned int game_id;	// index of game started by host
	unsigned int key;			// need this key to join the game
	char name[0];
} w3gs_game_info_1;

typedef struct w3gs_game_info_2 // 0x30
{
} w3gs_game_info_2;

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
	unsigned int game_id;	// index of game started by host
	unsigned int players;	// how many players are currently in the game
	unsigned int slots;		// how many slots in the game
} w3gs_refresh_game;

typedef struct w3gs_end_game // 0x33
{
	w3gs_header header;
	unsigned int game_id;
} w3gs_end_game;

typedef struct w3gs_map_check_1 // 0x3D
{
	w3gs_header header;
	unsigned int unknown;
	char path[0]; // again???
} w3gs_map_check_1;

typedef struct w3gs_map_check_2 // 0x3D
{
	unsigned int size;
	unsigned int crc;
	unsigned int csum;
	unsigned int sha1;
} w3gs_map_check_2;

typedef struct w3gs_map_size // 0x42
{
	w3gs_header header;
	unsigned int unknown;
	unsigned char flags;
	unsigned int size;
} w3gs_map_size;

typedef struct w3gs_ping // 0x01 and 0x46
{
	w3gs_header header;
	unsigned int tick;
} w3gs_ping;

#endif // _WAR3__H

