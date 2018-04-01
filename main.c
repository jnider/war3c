#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>
#include "war3.h"
#include "ll.h"
#include "maps.h"

#define NUM_DESCRIPTORS 3
#define MAX_PLAYERS 12
#define MAX_CMD_PARAMS 5
#define UDP_SOCKET 0
#define TCP_SOCKET 1
#define STDIN 2

#define GET_PARAMS(_c, _n) \
		for (i=1; i < _n+1; i++) \
		{ \
			_c[i] = strtok(NULL, " "); \
			if (!_c[i]) \
			{ \
				printf("Not enough params for cmd '%s' (expect %i got %i)\n", _c[0], _n+1, i); \
				return i; \
			} \
		}

enum actions
{
	ACTION_PING,
	ACTION_MAP_CHECK,
	ACTION_CONNECT_TCP,
	ACTION_SEND_JOIN_REQ,
	ACTION_SEARCH_GAME,
	ACTION_LOAD_GAME,
	ACTION_USER_CMD,
};

enum states
{
	STATE_FAILED,
	STATE_SEARCHING,	// waiting to see who is hosting a game
	STATE_FOUND,		// selected a game
	STATE_JOINING,
	STATE_JOINED,
	STATE_COUNTDOWN,
	STATE_PLAYING
};

enum w3c_player_type
{
	W3C_PLAYER_TYPE_OBSERVER,
	W3C_PLAYER_TYPE_HUMAN,
	W3C_PLAYER_TYPE_COMP_EASY,
	W3C_PLAYER_TYPE_COMP_NORM,
	W3C_PLAYER_TYPE_COMP_HARD,
};

enum
{
	CMD_SHOW,
	CMD_SET,
	CMD_QUIT,
	CMD_RESET,
	NUM_COMMANDS
};

typedef struct w3c_player_info
{
	int id;
	char* name;
	int type; // W3C_PLAYER_TYPE_
	int team;
	int colour;
	int race;
} w3c_player_info;

char* command_str[] = 
{
	"show",
	"set",
	"quit",
	"reset"
};

// from ka_array.c
extern const unsigned int ka_array[];
extern const unsigned int ka_size;

const int udp_port = 6112;
int state = STATE_SEARCHING;
//char mapname[64];
w3c_player_info player[MAX_PLAYERS];
int player_id; // my player id

typedef struct game_action
{
	int type;
	long param1;
	int param2;
} game_action;

char* w3gs_player_type(unsigned char type, unsigned char comp)
{
	switch(type)
	{
	case PLAYER_TYPE_HUMAN:
		return "Person";
	case PLAYER_TYPE_COMPUTER:
		switch(comp)
		{
		case COMPUTER_TYPE_EASY:
			return "Computer (easy)";
		case COMPUTER_TYPE_NORMAL:
			return "Computer (normal)";
		case COMPUTER_TYPE_HARD:
			return "Computer (hard)";
		}
	}
	return "Unknown";
}

char* w3gs_slot_status(unsigned char s)
{
	switch(s)
	{
	case SLOT_STATUS_OPEN:
		return "Open";
	case SLOT_STATUS_CLOSED:
		return "Closed";
	case SLOT_STATUS_OCCUPIED:
		return "Used";
	}
	return "Unknown";
}

char* w3gs_race(unsigned char r)
{
	if (r & 1<<RACE_HUMAN_SHIFT)
		return "Human";
	if (r & 1<<RACE_ORC_SHIFT)
		return "Orc";
	if (r & 1<<RACE_ELF_SHIFT)
		return "Elf";
	if (r & 1<<RACE_UNDEAD_SHIFT)
		return "Undead";
	if (r & 1<<RACE_FIXED_SHIFT)
		return "Fixed";
	if (r & 1<<RACE_RANDOM_SHIFT)
		return "Random";

	return "Unknown";
}

unsigned char w3gs_race_to_int(const char* c)
{
	if (strcasecmp(c, "human") == 0)
		return 1<<RACE_HUMAN_SHIFT;
	else if (strcasecmp(c, "orc") == 0)
		return 1<<RACE_ORC_SHIFT;
	else if (strcasecmp(c, "elf") == 0)
		return 1<<RACE_ELF_SHIFT;
	else if (strcasecmp(c, "undead") == 0)
		return 1<<RACE_UNDEAD_SHIFT;
	else if (strcasecmp(c, "fixed") == 0)
		return 1<<RACE_FIXED_SHIFT;
	else if (strcasecmp(c, "random") == 0)
		return 1<<RACE_RANDOM_SHIFT;
	return 0;
}

char* w3gs_int_to_colour(unsigned char c)
{
	switch(c)
	{
	case 0:
		return "Red";
	case 1:
		return "Blue";
	case 2:
		return "Cyan";
	case 3:
		return "Purple";
	case 4:
		return "Yellow";
	case 5:
		return "Orange";
	case 6:
		return "Green";
	case 7:
		return "Pink";
	case 8:
		return "Grey";
	case 9:
		return "Sky";
	case 10:
		return "Aqua";
	case 11:
		return "Brown";
	}
	return "Unknown";
}

char w3gs_colour_to_int(const char *c)
{
	if (strcasecmp(c, "red") == 0)
		return 0;
	if (strcasecmp(c, "blue") == 0)
		return 1;
	if (strcasecmp(c, "cyan") == 0)
		return 2;
	if (strcasecmp(c, "purple") == 0)
		return 3;
	if (strcasecmp(c, "yellow") == 0)
		return 4;
	if (strcasecmp(c, "orange") == 0)
		return 5;
	if (strcasecmp(c, "green") == 0)
		return 6;
	if (strcasecmp(c, "pink") == 0)
		return 7;
	if (strcasecmp(c, "grey") == 0)
		return 8;
	if (strcasecmp(c, "sky") == 0)
		return 9;
	if (strcasecmp(c, "aqua") == 0)
		return 10;
	if (strcasecmp(c, "brown") == 0)
		return 11;
	return 12;
}

static void w3gs_send_player_update(int sock, int player, int attr, int val)
{
	w3gs_player_update req;
	req.header.a = 0xF7;
	req.header.msg_id = W3GS_PLAYER_UPDATE;
	req.header.len = sizeof(w3gs_player_update);
	req.f1 = 1;
	req.f2 = 0xFF;
	req.player = player;
	req.attr = attr;
	req.val = val;
	send(sock, &req, sizeof(req), 0);
}

static int cmd_show(const char* what)
{
	int i;

	if (strcmp(what, "game") == 0)
	{
		//printf("Game server: %s\n", );
		puts("Player\tName\tTeam\tColour\tRace");

		for (i=0; i < MAX_PLAYERS; i++)
		{
			if (player[i].id)
			{
			printf("%i\t%s\t%i\t%s\t%s\n",
				player[i].id,
				player[i].name,
				player[i].team,
				w3gs_int_to_colour(player[i].colour),
				w3gs_race(player[i].race));
			}
		}
	}
	else
	{
		printf("Don't know how to show %s\n", what);
		return -1;
	}

	return 0;
}

static int cmd_set(int sock, const char* player, const char* attr, const char* val)
{
	int p, a, v;
	if (strcmp(attr, "team") == 0)
	{
		a = 0x11;
		v = atoi(val) - 1;
	}
	else if (strcmp(attr, "colour") == 0)
	{
		a = 0x12;
		v = w3gs_colour_to_int(val);
	}
	else if (strcmp(attr, "race") == 0)
	{
		a = 0x13;
		v = w3gs_race_to_int(val);
	}

	p = atoi(player);
	w3gs_send_player_update(sock, p, a, v);
	return 0;
}

static void post_search_game(int sock, unsigned int dest, char* buf, int version)
{
	int buf_len = sizeof(w3gs_search_game);
	struct sockaddr_in dstaddr;
	w3gs_search_game* search_game = (w3gs_search_game*)buf;
	search_game->header.a = 0xF7;
	search_game->header.msg_id = W3GS_SEARCH_GAME;
	search_game->header.len = sizeof(w3gs_search_game);
	strncpy(search_game->product, BLIZZARD_ID_WARCRAFT_3, 4);
	search_game->version = version;
	search_game->unknown = 0;

	//printf("post search game version=0x%x\n", version);
	dstaddr.sin_family = AF_INET;
	dstaddr.sin_port = htons(udp_port);
	dstaddr.sin_addr.s_addr = dest;
	int err = sendto(sock, buf, buf_len, 0, (struct sockaddr*)&dstaddr, sizeof(struct sockaddr));
	if (err == -1)
		printf("Error posting search game msg\n");
}

static void send_pong(int sock, unsigned int tick)
{
	w3gs_ping req;
	req.header.a = 0xF7;
	req.header.msg_id = W3GS_PONG;
	req.header.len = sizeof(w3gs_ping);
	req.tick = tick;
	send(sock, &req, sizeof(req), 0);
}

static void send_join_req(int sock, int game_id, int key)
{
	w3gs_req_join req;
	struct sockaddr_in addr;
	int socksize = sizeof(struct sockaddr_in);

	getsockname(sock, (struct sockaddr*)&addr, &socksize);

	req.header.a = 0xF7;
	req.header.msg_id = W3GS_REQ_JOIN;
	req.header.len = sizeof(w3gs_req_join);
	req.game_id = game_id;
	req.key = key;
	req.unknown = 0;
	req.port = 6112;
	req.peer_key = 0xa;
	strcpy(req.name, "ifc6410");
	req.unknown2 = 1;
	req.unknown3 = 2;
	req.int_port = addr.sin_port;
	req.int_ip = addr.sin_addr.s_addr;
	req.unknown4 = 0;
	req.unknown5 = 0;
	send(sock, &req, sizeof(req), 0);
}

static void send_map_size(int sock, int size)
{
	w3gs_map_size req;
	req.header.a = 0xF7;
	req.header.msg_id = W3GS_MAP_SIZE;
	req.header.len = sizeof(w3gs_map_size);
	req.unknown = 1;
	req.flags = 1;
	req.size = size;
	send(sock, &req, sizeof(req), 0);
}

static void send_game_loaded(int sock, unsigned char player)
{
	w3gs_player_loaded req;
	req.header.a = 0xF7;
	req.header.msg_id = W3GS_PLAYER_LOADED;
	req.header.len = sizeof(w3gs_player_loaded);
	req.player = player;
	send(sock, &req, sizeof(req), 0);
}

static void add_action(linked_list* list, int type, long param1, int param2)
{
	game_action* action = malloc(sizeof(game_action));
	action->type = type;
	action->param1 = param1;
	action->param2 = param2;
	//printf("Adding action %i\n", type);
	ll_add(list, action);
}

static void handle_message(int msg_id, char* buf, struct sockaddr_in* inaddr, linked_list* actions_list)
{
	int i;
	int version=26;
	char inaddr_str[16];
	char product[5];

	w3gs_search_game* search_game;
	w3gs_game_info_1* game_info;
	w3gs_slot_info_join_1* slot_info;
	w3gs_slot_info_join_2* slot_info_2;
	w3gs_create_game* create_game;
	w3gs_refresh_game* refresh_game;
	w3gs_end_game* end_game;
	w3gs_slot_info* slot;
	w3gs_player_info_1* pi1;
	w3gs_player_info_2* pi2;
	w3gs_map_check_1* mc1;
	w3gs_map_check_2* mc2;
	w3gs_ping* ping;
	game_action* action;
	w3gs_player_left* left;
	w3gs_player_loaded* loaded;

	inet_ntop(AF_INET, &inaddr->sin_addr, inaddr_str, 16);

	switch(msg_id)
	{
	case W3GS_PING:
		//printf("Ping!\n");
		ping = (w3gs_ping*)buf;
		add_action(actions_list, ACTION_PING, ping->tick, 0);
		break;

	case W3GS_PLAYER_INFO:
		pi1 = (w3gs_player_info_1*)buf;
		pi2 = (w3gs_player_info_2*)(pi1->name + strlen(pi1->name) + 1);
		//inet_ntop(AF_INET, &pi2->ext_addr, inaddr_str, 16);
		//printf("Player %i (%s)\n", pi1->player, pi1->name);
		player[pi1->player - 1].name = strdup(pi1->name);
		break;

	case W3GS_PLAYER_LEFT:
		left = (w3gs_player_left*)buf;
		printf("%s left the game\n", player[left->player-1].name);
		break;

	case W3GS_PLAYER_LOADED:
		loaded = (w3gs_player_loaded*)buf;
		printf("%s finished loading\n", player[loaded->player-1].name);
		break;

	case W3GS_KICKED:
		printf("You were kicked out\n");
		break;

	case W3GS_UNKNOWN2:
		break;

	case W3GS_SLOT_INFO:
		//printf("Got slot info\n");
		slot_info = (w3gs_slot_info_join_1*)buf;
		//printf("slots: %i\n", slot_info->num_slots);
		slot = slot_info->slot;
		//printf("Player\tType\tStatus\tTeam\tColor\tRace\n");
		for (i=0; i < slot_info->num_slots; i++)
		{
			if (slot->type == PLAYER_TYPE_HUMAN)
				player[i].type = W3C_PLAYER_TYPE_HUMAN;
			else
			{
				switch(slot->computer_type)
				{
				case COMPUTER_TYPE_EASY:
					player[i].type = W3C_PLAYER_TYPE_COMP_EASY; break;
				case COMPUTER_TYPE_NORMAL:
					player[i].type = W3C_PLAYER_TYPE_COMP_NORM; break;
				case COMPUTER_TYPE_HARD:
					player[i].type = W3C_PLAYER_TYPE_COMP_HARD; break;
				}
			}
			player[i].id = slot->player;
			player[i].team = slot->team + 1;
			player[i].colour = slot->color;
			player[i].race = slot->race;
/*
			printf("%i\t%s\t%s\t%i\t%s\t%s\n", slot->player,
				w3gs_player_type(slot->type, slot->computer_type),
				w3gs_slot_status(slot->status),
				slot->team < slot_info->num_slots ? slot->team + 1 : -1,
				w3gs_int_to_colour(slot->color),
				w3gs_race(slot->race));
*/
			slot++;
		}
		break;

	case W3GS_COUNTDOWN_START:
		printf("Countdown started\n");
		state = STATE_COUNTDOWN;
		break;

	case W3GS_COUNTDOWN_END:
		printf("Countdown ended\n");
		state = STATE_PLAYING;
		add_action(actions_list, ACTION_LOAD_GAME, 0, 0);
		break;

	case W3GS_OUTGOING_ACTION:
		printf("Got outgoing action\n");
		break;

	case W3GS_MAP_CHECK:
		mc1 = (w3gs_map_check_1*)buf;
		mc2 = (w3gs_map_check_2*)(mc1->path + strlen(mc1->path) + 1);
		if (state == STATE_JOINED)
		{
			printf("Check map @ %s size=%i crc=0x%x sha1=0x%x\n", mc1->path, mc2->size, mc2->crc, mc2->sha1);
			char* path = strdup(mc1->path);
			add_action(actions_list, ACTION_MAP_CHECK, (unsigned long)path, mc2->size);
		}
		break;

	case W3GS_SLOT_INFO_JOIN:
		//printf("Got slot info join\n");
		state = STATE_JOINED;
		slot_info = (w3gs_slot_info_join_1*)buf;
		//printf("length: %i\n", slot_info->length);
		//printf("slots: %i\n", slot_info->num_slots);
		slot = slot_info->slot + slot_info->num_slots;
/*
		printf("Player\tType\tStatus\tTeam\tColor\tRace\n");
		for (i=0; i < slot_info->num_slots; i++)
		{
			printf("%i\t%s\t%s\t%i\t%s\t%s\n", slot->player,
				w3gs_player_type(slot->type, slot->computer_type),
				w3gs_slot_status(slot->status),
				slot->team,
				w3gs_int_to_colour(slot->color),
				w3gs_race(slot->race));
			slot++;
		}
*/
		slot_info_2 = (w3gs_slot_info_join_2*)slot;
/*
		printf("size: %i used %i\n", slot_info->header.len,
			sizeof(w3gs_slot_info) * slot_info->num_slots-1
			+ sizeof(w3gs_slot_info_join_1)
			+ sizeof(w3gs_slot_info_join_2));
		printf("Timestamp: 0x%x\n", slot_info_2->timestamp);
*/
		break;

	case W3GS_SEARCH_GAME:
		search_game = (w3gs_search_game*)buf;
		strncpy(product, (char*)&search_game->product, 4);
		product[4] = 0;
		printf("Game search request - Product: %s\nVersion: 0x%x\n", product, search_game->version);
		break;

	case W3GS_GAME_INFO:
		game_info = (w3gs_game_info_1*)buf;
		printf("Found game %s (%i) on %s\n", game_info->name, game_info->game_id, inaddr_str);
		//printf("Server: %s\n", inaddr_str);
		//printf("Game ID: %i\n", game_info->game_id);
		//printf("Version: %i\n", game_info->version);
		//printf("Key: 0x%x\n", game_info->key);
		//printf("Name: %s\n", game_info->name);
		if (state == STATE_SEARCHING)
		{
			state = STATE_FOUND;
			add_action(actions_list, ACTION_CONNECT_TCP, inaddr->sin_addr.s_addr, 0);
			add_action(actions_list, ACTION_SEND_JOIN_REQ, game_info->game_id, game_info->key);
		}
		break;

	case W3GS_CREATE_GAME:
		create_game = (w3gs_create_game*)buf;
		printf("%s created game %i, version=%i\n", inaddr_str, create_game->counter, create_game->version);
		version = create_game->version;
		break;

	case W3GS_REFRESH_GAME:
		refresh_game = (w3gs_refresh_game*)buf;
		if (state == STATE_SEARCHING)
			add_action(actions_list, ACTION_SEARCH_GAME, inaddr->sin_addr.s_addr, version);
		break;

	case W3GS_END_GAME:
		end_game = (w3gs_end_game*)buf;
		if (state != STATE_COUNTDOWN)
			printf("Game %i on %s has ended\n", end_game->game_id, inaddr_str);
		break;

	default:
		printf("Got unknown message 0x%x\n", msg_id);
	}
}

static void read_network(int socket, char* buf, int bufsize, linked_list* actions_list)
{
	int len;
	struct sockaddr_in inaddr;
	int inaddr_len = sizeof(struct sockaddr_in);

	//printf("Waiting for broadcast\n");
	len = recvfrom(socket, buf, bufsize, 0, (struct sockaddr*)&inaddr, &inaddr_len);
	if (len < 0)
		printf("Error %i receiving\n", len);

	void* pp = buf; // packet pointer, we may have multiple packets in a buffer
	//int i;
	while ((char*)pp < (char*)buf + len)
	{
		w3gs_header* h = (w3gs_header*)pp;
		handle_message(h->msg_id, pp, &inaddr, actions_list);
		pp += h->len;
	}
}

static void read_stdin(int fd, char* buf, int buflen, int len, linked_list* actions_list)
{
	char temp[65];

	//printf("read_stdin %i bytes available\n", len);
	while (len)
	{
		// figure out how much to read
		int to_read = len > 64 ? 64 : len;
		int actual = read(fd, temp, to_read);
		if (actual < 0)
			return;
		temp[actual] = 0;
		if (strlen(buf) + strlen(temp) > buflen)
		{
			printf("Cmd buffer overflow\n");
			return;
		}

		// check for a \n, and ignore anything that may be after it
		char* eoc = strchr(temp, '\n');
		if (eoc)
			*eoc = 0;
		strcat(buf, temp);
		if (eoc)
			add_action(actions_list, ACTION_USER_CMD, 0, 0);
		len -= actual;
	}

	//printf("CMDBUF: %s\n", buf);
}

static int handle_command(int fd, char* cmdline)
{
	char* cmd[MAX_CMD_PARAMS];
	int i;
	int cmd_id = -1;

	cmd[0] = strtok(cmdline, " ");
	if (!cmd[0])
		return 0;
	
	for (i=0; i < NUM_COMMANDS; i++)
	{
		if (strcmp(cmd[0], command_str[i]) == 0)
		{
			cmd_id = i;
			break;
		}
	}

	if (cmd_id == -1)
	{
		printf("Unknown command: %s\n", cmd[0]);
		return 1;
	}

	switch(cmd_id)
	{
	case CMD_SHOW:
		GET_PARAMS(cmd, 1);
		cmd_show(cmd[1]);
		break;

	case CMD_SET:
		GET_PARAMS(cmd, 3);
		cmd_set(fd, cmd[1], cmd[2], cmd[3]);
		break;

	case CMD_RESET:
		printf("resetting\n");
		state = STATE_SEARCHING;
		break;

	case CMD_QUIT:
		return -1;
	}

	return 0;
}
 
int main(int argc, char** argv)
{
	struct sockaddr_in sockaddr, srvaddr;
	int err;

	printf("Warcraft III client\n");

	// create the socket
	printf("Listen on UDP %i\n", udp_port);
	int udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (udp == -1)
	{
		printf("Error creating udp socket\n");
		return -1;
	}

	// bind
	memset(&sockaddr, 0, sizeof(sockaddr));

	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(udp_port);
	sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	err = bind(udp, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
	if (err == -1)
	{
		printf("Error %i binding udp listener\n", errno);
		close(udp);
		return -1;
	}

	// wait for a message
	linked_list* actions_list = ll_init();
	int bufsize = 256;
	int cmdbufsize = 256;
	char* buf = malloc(bufsize);
	char* cmdbuf = malloc(cmdbufsize);
	char inaddr_str[16];
	int len;
	game_action* action;
	int quit = 0;
	char* mapbase = "."; ///mnt/badger/games/war3ft/";
	//char* mapbase = "/mnt/data/public/games/war3ft/";

	struct pollfd fd[NUM_DESCRIPTORS];
	fd[UDP_SOCKET].fd = udp;
	fd[UDP_SOCKET].events = POLLIN;
	fd[TCP_SOCKET].fd = -1;
	fd[TCP_SOCKET].events = POLLIN;
	fd[STDIN].fd = STDIN_FILENO;
	fd[STDIN].events = POLLIN;

	puts("> ");

	while (!quit)
	{
		// check for incoming network information
		int count = poll(fd, NUM_DESCRIPTORS, 0); // return immediately
		if (count > 0)
		{
			if (fd[UDP_SOCKET].revents & POLLIN)
				read_network(fd[UDP_SOCKET].fd, buf, bufsize, actions_list);
			if (fd[TCP_SOCKET].revents & POLLIN)
				read_network(fd[TCP_SOCKET].fd, buf, bufsize, actions_list);
			if (fd[STDIN].revents & POLLIN)
				read_stdin(fd[STDIN].fd, cmdbuf, cmdbufsize, count, actions_list);
		}
		// perform any delayed actions
		while (action = ll_pop(actions_list))
		{
			//printf("Got action %i\n", action->type);
			switch(action->type)
			{
			case ACTION_PING:
				send_pong(fd[TCP_SOCKET].fd, action->param1);
				break;

			case ACTION_MAP_CHECK:
				err = load_map((char*)action->param1, mapbase);
				if (err < 0)
				{
					printf("Map no good (%i)\n", err);
					state = STATE_FAILED;
				}
				else
				{
					printf("Map ok\n");
					send_map_size(fd[TCP_SOCKET].fd, err);
				}
				free((char*)action->param1); // this was a duplicated string
				break;

			case ACTION_CONNECT_TCP:
				printf("Opening socket\n");
				fd[TCP_SOCKET].fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (fd[TCP_SOCKET].fd != -1)
				{
					inet_ntop(AF_INET, &action->param1, inaddr_str, 16);
					printf("Connecting socket to %s\n", inaddr_str);
					srvaddr.sin_family = AF_INET;
					srvaddr.sin_addr.s_addr = action->param1;
					srvaddr.sin_port = htons(udp_port);
					err = connect(fd[TCP_SOCKET].fd, (struct sockaddr*)&srvaddr, sizeof(struct sockaddr_in));
					if (err == 0)
					{
						//printf("Socket connected\n");
						state = STATE_JOINING;
					}
					else
					{
						printf("Error connecting tcp socket\n");
						close(fd[TCP_SOCKET].fd);
						fd[TCP_SOCKET].fd = -1;
					}
				}
				else
					printf("Error creating tcp socket\n");
				break;

			case ACTION_SEND_JOIN_REQ:
				send_join_req(fd[TCP_SOCKET].fd, action->param1, action->param2);
				break;

			case ACTION_SEARCH_GAME:
				post_search_game(fd[UDP_SOCKET].fd, action->param1, buf, action->param2);
				break;

			case ACTION_LOAD_GAME:
				send_game_loaded(fd[TCP_SOCKET].fd, 2);
				break;

			case ACTION_USER_CMD:
				if (handle_command(fd[TCP_SOCKET].fd, cmdbuf) < 0)
					quit = 1;
				cmdbuf[0] = 0;
				break;
			}
			free(action);
		}

		// update the gui
	}

	free(buf);
			printf("buf freed\n");
	free(cmdbuf);
			printf("cmdbuf freed\n");
	return 0;
}

