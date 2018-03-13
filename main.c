#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "war3.h"

enum
{
	SEARCHING,
	FOUND,
	JOINING,
	PLAYING
};

const int udp_port = 6112;
int status = SEARCHING;

static void post_search_game(int sock, unsigned int dest, char* buf, int version, int slot)
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

	printf("post search game version=0x%x\n", version);
	dstaddr.sin_family = AF_INET;
	dstaddr.sin_port = htons(udp_port);
	dstaddr.sin_addr.s_addr = dest;
	int err = sendto(sock, buf, buf_len, 0, (struct sockaddr*)&dstaddr, sizeof(struct sockaddr));
	if (err == -1)
		printf("Error posting search game msg\n");
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
	//req.unknown6 = 0;
	send(sock, &req, sizeof(req), 0);
}

int main(int argc, char** argv)
{
	struct sockaddr_in sockaddr, inaddr, srvaddr;
	int err;
	int tcp; // the TCP socket
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
	int bufsize = 100;
	char* buf = malloc(bufsize);
	int inaddr_len = sizeof(struct sockaddr_in);
	int len;
	int version=26;
	char product[5];
	char inaddr_str[16];

	w3gs_search_game* search_game;
	w3gs_game_info* game_info;
	w3gs_create_game* create_game;
	w3gs_refresh_game* refresh_game;
	w3gs_end_game* end_game;
	while (status == SEARCHING)
	{
		//printf("Waiting for broadcast\n");
		len = recvfrom(udp, buf, bufsize, 0, (struct sockaddr*)&inaddr, &inaddr_len);
		if (len < 0)
			printf("Error %i receiving\n", len);

		w3gs_header* h = (w3gs_header*)buf;
		inet_ntop(AF_INET, &inaddr.sin_addr, inaddr_str, 16);
		switch(h->msg_id)
		{
		case W3GS_SEARCH_GAME:
			search_game = (w3gs_search_game*)buf;
			strncpy(product, (char*)&search_game->product, 4);
			product[4] = 0;
			printf("Search game - Product: %s\nVersion: 0x%x\n", product, search_game->version);
			break;

		case W3GS_GAME_INFO:
			game_info = (w3gs_game_info*)buf;
			printf("Got game info\n");
			printf("Server: %s\n", inaddr_str);
			printf("Game ID: %i\n", game_info->game_id);
			printf("Version: %i\n", game_info->version);
			printf("Key: 0x%x\n", game_info->key);
			printf("Name: %s\n", game_info->name);
			tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (tcp != -1)
			{
				srvaddr.sin_family = AF_INET;
				srvaddr.sin_addr = inaddr.sin_addr;
				srvaddr.sin_port = htons(udp_port);
				err = connect(tcp, (struct sockaddr*)&srvaddr, sizeof(struct sockaddr_in));
				if (err == 0)
				{
					status = JOINING;
					send_join_req(tcp, game_info->game_id, game_info->key);
				}
				else
				{
					printf("Error connecting tcp socket\n");
					close(tcp);
					tcp = -1;
				}
			}
			else
				printf("Error creating tcp socket\n");
			break;

		case W3GS_CREATE_GAME:
			create_game = (w3gs_create_game*)buf;
			printf("%s created game %i, version=%i\n", inaddr_str, create_game->counter, create_game->version);
			version = create_game->version;
			break;

		case W3GS_REFRESH_GAME:
			refresh_game = (w3gs_refresh_game*)buf;
			printf("Game %i has %i/%i\n", refresh_game->counter, refresh_game->players, refresh_game->slots);
			post_search_game(udp, inaddr.sin_addr.s_addr, buf, version, 1);
			break;

		case W3GS_END_GAME:
			end_game = (w3gs_end_game*)buf;
			printf("Game %i on %s has ended\n", end_game->counter, inaddr_str);
			break;

		default:
			printf("Got unknown message 0x%x from %s\n", h->msg_id, inaddr_str);
		}
	}

	// now the TCP part
	while (status == JOINING)
	{
		printf("Joining game\n");

		// wait for a slot info join message
		recv(tcp, buf, bufsize, 0);
		w3gs_header* h = (w3gs_header*)buf;
		switch(h->msg_id)
		{
		case W3GS_SLOT_INFO_JOIN:
			printf("Got slot info join\n");
			break;

		case W3GS_GAME_INFO:
			game_info = (w3gs_game_info*)buf;
			//printf("Got game info\n");
			//printf("Server: %s\n", inaddr_str);
			//printf("Game ID: %i\n", game_info->game_id);
			//printf("Version: %i\n", game_info->version);
			//printf("Key: 0x%x\n", game_info->key);
			//printf("Name: %s\n", game_info->name);
			printf("Join failed\n");
			status = SEARCHING;
			break;

		default:
			printf("Got unknown message 0x%x\n", h->msg_id);
		}
	}

	close(tcp);

	return 0;
}

