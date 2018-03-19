#ifndef _MAPS__H
#define _MAPS__H

#define MAP_MAGIC "HM3W" // W3MH
#define BLP_MAGIC "BLP1"

#define W3I_FORMAT_ROC 18
#define W3I_FORMAT_TFT 25

#define BLP_FLAG_ALPHA (1<<BLP_FLAG_ALPHA_SHIFT)

enum blp_type
{
	BLP_TYPE_JPG,
	BLP_TYPE_PALETTE
};

enum blp_flag_shift
{
	BLP_FLAG_ALPHA_SHIFT=3
};

enum file_flag_shift
{
	W3M_FILE_FLAG_HIDE_MINIMAP_SHIFT,
	W3M_FILE_FLAG_MODIFY_ALLY_SHIFT,
	W3M_FILE_FLAG_MELEE_SHIFT,
	W3M_FILE_FLAG_LARGE_SHIFT,
	W3M_FILE_FLAG_PARTIAL_SHIFT,
	W3M_FILE_FLAG_FIXED_SHIFT,
	W3M_FILE_FLAG_CUST_FORCES_SHIFT,
	W3M_FILE_FLAG_CUST_TECH_SHIFT,
	W3M_FILE_FLAG_CUST_ABILITY_SHIFT,
	W3M_FILE_FLAG_CUST_UPGRADE_SHIFT,
	W3M_FILE_FLAG_PROPS_OPEN_SHIFT,
	W3M_FILE_FLAG_WATER_CLIFF_SHIFT,
	W3M_FILE_FLAG_WATER_SHORE_SHIFT,
};

#define W3M_FILE_FLAG_HIDE_MINIMAP (1<<W3M_FILE_FLAG_HIDE_MINIMAP_SHIFT)

typedef struct w3m_file_header
{
	char magic[4]; // see MAP_MAGIC
	unsigned int unknown;
	char name[0];
} w3m_file_header;

typedef struct w3m_file_header_2
{
	int flags; // see W3M_FILE_FLAG_
	int max_players;
} w3m_file_header_2;

typedef struct w3i_header_cmn
{
	unsigned int format; // see W3I_FORMAT_
	int saves;
	int editor_ver;
} w3i_header_cmn;

typedef struct w3i_header_roc_1
{
	float cam_bounds[8];
	int cam_bounds_cmpl[4];
	char width[4];
	unsigned int height;
	unsigned int flags;
	char type;
	unsigned int bg; // campaign background num
} w3i_header_roc_1;

typedef struct w3i_header_roc_2
{
	char num_players[4];
} w3i_header_roc_2;

typedef struct w3i_slot_info
{
	char colour[4];
	char status[4];
	char race[4];
	int start_pos;
	int start_x;
	int start_y;
	int ally_low;
	int ally_high;
} w3i_slot_info;

typedef struct w3i_header_cmn_2
{
	char teams[4];
} w3i_header_cmn_2;

typedef struct w3i_team_info
{
	char flags[4];
	char pmask[4]; // player mask
	char team_name[0];
} w3i_team_info;

typedef struct w3i_roc_header
{
	struct w3i_header_cmn* a;
	char* name;
	char* author;
	char* description;
	char* rnp; // recommended number of players
	struct w3i_header_roc_1* b;
	char* ml_text;
	char* ml_title;
	char* ml_subtitle;
	int* map_loading_bg; // screen num
	char* prologue_screen_text;
	char* prologue_screen_title;
	char* prologue_screen_subtitle;
	w3i_header_roc_2* c;
	w3i_slot_info* slot;
	w3i_header_cmn_2* d;
	w3i_team_info* team;
} w3i_roc_header;

typedef struct blp_header
{
	char magic[4]; // see BLP_MAGIC
	int type; // see BLP_TYPE_
	int flags; // see BLP_FLAGS_
	int width;
	int height;
	int colour; // see BLP_COLOUR_
	int u1;
	int mipmap_offset[16];
	int mipmap_size[16];
} blp_header;

typedef struct blp_header_jpg
{
	blp_header a;
	int jpg_header_size;
	char jpg_header[0];
	// immediately followed by jpg data
} blp_header_jpg;

int load_map(char* path);

#endif // _MAPS__H

