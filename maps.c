#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "maps.h"
#include "StormLib.h"

typedef struct string_entry
{
	unsigned int str_id; // the string id from the map TRIGSTR_%i
	unsigned int offset; // offset into the ELF-style string table
} string_entry;

typedef struct string_table
{
	int size; // size of strings
	int max_entries; // size of index
	int count; // number of entries
	char* strings; // string data
	string_entry* index; // lookup table to find offset into string data
} string_table;

static int get_string_id(const char* s)
{
	char* num = strchr(s, '_');
	if (!num)
		return 0;
	return atoi(++num);
}

const char* lookup_string(int id, string_table* tbl)
{
	//printf("%i entries\n", tbl->count);

	int i;
	for (i=0; i < tbl->count; i++)
	{
		if (tbl->index[i].str_id == id)
		{
			//printf("[%i|%i] %s (%i)\n", tbl->index[i].str_id, tbl->index[i].offset, tbl->strings + tbl->index[i].offset, strlen(tbl->strings + tbl->index[i].offset));
			return tbl->strings + tbl->index[i].offset;
		}
	}

	return NULL;
}

// wts contains the string table
static int wts_read(HANDLE h, string_table* tbl)
{
	int ret=0;

	int fsize = SFileGetFileSize(h, NULL);

	char* buf = malloc(fsize);
	int read;
	if (!SFileReadFile(h, buf, fsize, &read, 0))
	{
		printf("Error reading wts\n");
		ret = -1;
		goto err;
	}

	// dump to external file
	FILE* outfile = fopen("strings.txt", "wb");
	if (outfile)
	{
		fwrite(buf, fsize, 1, outfile);
		fclose(outfile);
		free(buf);
	}

	// build a string table like ELF, plus an index to find the entries
	tbl->size = 65536; // allocate 64KB for strings - that should be enough for anyone
	tbl->max_entries = 256;
	tbl->count = 0;
	tbl->strings = malloc(tbl->size); 
	tbl->index = malloc(sizeof(string_entry) * tbl->max_entries);
	tbl->strings[0] = 0; // leave the first byte null for the null string
	char* entry = tbl->strings + 1;
	string_entry* index = tbl->index;
	
	char* tok = strtok(buf, " ");
	while (tok)
	{
		tok = strtok(NULL, "{");
		if (!tok) break;

		int id = atoi(tok);
		//printf("id: %i\n", id);

		// take the value and peel off any leading or trailing whitespace
		tok = strtok(NULL, "}");
		if (!tok) break;
		while (isspace(*tok)) tok++;
		char* end = tok + strlen(tok) - 1;
  		while(end > tok && isspace(*end)) end--;
		*(end+1) = 0;
		//printf("val: %s\n", tok);

		// add the entry to the index
		index = &tbl->index[tbl->count++];
		index->str_id = id;
		index->offset = 0; // point to the 0-string

		if (strlen(tok) > 0)
		{
			if (entry - tbl->strings + strlen(tok) + 1 > tbl->size)
				printf("Error: Overflow in string table\n");

			index->offset = entry - tbl->strings;

			// add the string to the table
			//printf("Adding %s (%i) to string table\n", tok, strlen(tok));
			strcpy(entry, tok);
			entry += strlen(tok) + 1;
		}

		tok = strtok(NULL, " ");
	}
	printf("Done\n");

err:
	return ret;
}

// blp contains the minimap
static int blp_read(HANDLE h)
{
	char* buf;
	int ret=0;
	blp_header* header;
	printf("Reading minimap\n");

	buf = malloc(256);
	header = (blp_header*)buf;

	int read;
	if (!SFileReadFile(h, buf, sizeof(blp_header_jpg), &read, 0))
	{
		printf("Error reading blp\n");
		ret = -1;
		goto err;
	}

	if (memcmp(header->magic, BLP_MAGIC, 4) != 0)
	{
		printf("blp bad magic %s\n", header->magic);
		ret = -2;
		goto err;
	}

	if (header->type & BLP_TYPE_PALETTE)
		printf("paletted\n");
	else
	{
		blp_header_jpg* blp_jpg = (blp_header_jpg*)buf;
		//printf("jpg header size 0x%x\n", blp_jpg->jpg_header_size);
		
		char* jpg = malloc(blp_jpg->jpg_header_size + blp_jpg->a.mipmap_size[0]); // JPEG header + data
		if (!SFileReadFile(h, jpg, blp_jpg->jpg_header_size, &read, 0))
		{
			printf("Error reading jpg header\n");
			ret = -3;
			goto err;
		}
		SFileSetFilePointer(h, blp_jpg->a.mipmap_offset[0], NULL, FILE_BEGIN);
		if (!SFileReadFile(h, jpg+blp_jpg->jpg_header_size, blp_jpg->a.mipmap_size[0], &read, 0))
		{
			printf("Error reading jpg data\n");
		}

		// dump to external file
		FILE* mmjpg = fopen("minimap.jpg", "wb");
		if (mmjpg)
		{
			fwrite(jpg, blp_jpg->jpg_header_size + blp_jpg->a.mipmap_size[0], 1, mmjpg);
			fclose(mmjpg);
			free(jpg);
		}
		printf("Done\n");
	}

err:
	free(buf);
	return ret;
}

// w3i contains map info
static int w3i_read(HANDLE h, string_table* tbl)
{
	char* buf;
	int ret = -1;
	unsigned int len = SFileGetFileSize(h, 0);
	printf("war3map.w3i size: %u\n", len);

	buf = malloc(len);

	int read;
	if (!SFileReadFile(h, buf, len, &read, 0))
	{
		printf("Error reading w3i\n");
		ret = -1;
		goto err;
	}

	w3i_header_cmn* header = (w3i_header_cmn*)buf;
	w3i_roc_header roc;
	switch(header->format)
	{
	case W3I_FORMAT_ROC:
		printf("Reign Of Chaos map\n");
		roc.a = header;
		roc.name = buf + sizeof(w3i_header_cmn);
		roc.author = roc.name + strlen(roc.name) + 1;
		roc.description = roc.author + strlen(roc.author) + 1;
		roc.rnp = roc.description + strlen(roc.description) + 1;
		roc.b = (w3i_header_roc_1*)roc.rnp + strlen(roc.rnp) + 1;
		roc.ml_text = (char*)(roc.b + sizeof(w3i_header_roc_1));
		roc.ml_title = roc.ml_text + strlen(roc.ml_text) + 1;
		roc.ml_subtitle = roc.ml_title + strlen(roc.ml_title) + 1;
		printf("name: %s\n", lookup_string(get_string_id(roc.name), tbl));
		printf("author: %s\n", lookup_string(get_string_id(roc.author), tbl));
		printf("description: %s\n", lookup_string(get_string_id(roc.description), tbl));
		printf("recommended players: %s\n", lookup_string(get_string_id(roc.rnp), tbl));
		//printf("Size: %i %i %i %i\n", roc.b->width[0], roc.b->width[1], roc.b->width[2], roc.b->width[3]);
		//printf("ml text: %s\n", roc.ml_text);
		break;

	case W3I_FORMAT_TFT:
		printf("Frozen Throne map\n");
		break;

	default:
		printf("Unknown map format %i\n", header->format);
		ret = -2;
		goto err;
	}
	
	ret = 0;

err:
	//free(buf);
	return ret;
}

int load_map(char* path)
{
	int bufsize = 512;
	char* buf;
	w3m_file_header* header;
	w3m_file_header_2* header2;
	string_table trigstr;

	// fix up the path for POSIX from Windows
	char* slash = strchr(path, '\\');
	while (slash)
	{
		*slash = '/';
		slash = strchr(path, '\\');
	}

	printf("Loading map from %s\n", path);

	int size;
	HANDLE MapMPQ;
	HANDLE SubFile;
	FILE* mf = fopen(path, "rb");
	if (!mf)
	{
		printf("Error opening map at %s\n", path);
		return -1;
	}

	printf("map is open!\n");

	fseek(mf, 0, SEEK_END);
	size = ftell(mf);
	printf("map is %i bytes\n", size);
	fseek(mf, 0, SEEK_SET);

	buf = malloc(bufsize);
	fread(buf, bufsize, 1, mf);
	header = (w3m_file_header*)buf;
	header2 = (w3m_file_header_2*)(buf + sizeof(w3m_file_header) + strlen(header->name) + 1);

	printf("Magic: %s\n", header->magic);
	printf("Map Name: %s\n", header->name);
	printf("Max Players: %i\n", header2->max_players);
	printf("Flags: 0x%x\n", header2->flags);

	fclose(mf);

	if (!SFileOpenArchive(path, 0, MPQ_OPEN_FORCE_MPQ_V1, &MapMPQ))
		return -2;

	//if (!SFileOpenFileEx(MapMPQ, "Scripts\\common.j", 0, &SubFile))
	//	return -3;
	//printf("found common.j\n");

	// read the string table
	printf("Reading wts\n");
	if (SFileOpenFileEx(MapMPQ, "war3map.wts", 0, &SubFile))
	{
		wts_read(SubFile, &trigstr);
		SFileCloseFile(SubFile);
	}

	// read the map info
	printf("Reading w3i\n");
	if (SFileOpenFileEx(MapMPQ, "war3map.w3i", 0, &SubFile))
	{
		w3i_read(SubFile, &trigstr);
		SFileCloseFile(SubFile);
	}

	// read the minimap
	printf("Reading blp\n");
	if (SFileOpenFileEx(MapMPQ, "war3mapMap.blp", 0, &SubFile))
	{
		blp_read(SubFile);
		SFileCloseFile(SubFile);
	}

	SFileCloseArchive(MapMPQ);

	return size;
}

