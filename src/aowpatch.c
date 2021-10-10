#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include "progress.h"

/* */
static const unsigned char pattern[] = {
   0x66, 0x8B, 0x16, 0x8D,
   0x74, 0x16, 0x03, 0x81,
   0xE6, 0xFC, 0xFF, 0xFF,
   0x0F, 0x48, 0x75, 0xF0,
};

/* */
static const unsigned char replacement[] = {
	0x90, 0x90, 0x90, 0x90,
	0x90, 0x90, 0x90, 0x90,
	0x90, 0x90, 0x90, 0x90,
	0x90, 0x90, 0x90, 0x90,
};

int exit_status = EXIT_SUCCESS;

int apply_patch(const char* filename); 

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		fprintf(stderr, "Usage: %s  -  %s\n", argv[0], ".dpl file to patch");
		exit(EXIT_FAILURE);
	}

	char* filename = argv[1];

	exit_status = apply_patch(filename);

	exit(exit_status);
}

int apply_patch(const char* filename) 
{
	int exit_status = EXIT_SUCCESS;
    	size_t read = 0;
    	size_t written = 0;
    	size_t size = 0;
	uint32_t pattern_count = 0;
	uint32_t replacement_count = 0;
	off_t pattern_offsets[4] = { 0 };
	off_t replacement_offsets[4] = { 0 };
	struct stat sb;
	FILE* file = NULL;
	uint8_t* data = NULL;

	/* check for existence and get file size */
	if((exit_status = stat(filename, &sb)) != 0)
	{
		fprintf(stderr, "ERROR: Source file not found: %s %d!\n", filename, exit_status);
		exit(EXIT_FAILURE);
	}
	size = sb.st_size;

	/* open file */
	file = fopen(filename, "rb");
	if(!file)
    	{
    		fprintf(stderr, "ERROR: Cannot open source file: %s!\n", filename);
    		exit(EXIT_FAILURE);
    	}

	/* allocate data of size for file */
	data = calloc(1, sb.st_size * sizeof(uint8_t));

	/* read data in quad part chunks of 512 bytes and display progress */
	file_block_t* ptr = (file_block_t*)&data[0];
	for(read = 0; read < size; read += fread(ptr++, sizeof(uint8_t), sizeof(file_block_t), file))
		status_progress_update("Loading AOW patch file", read, size);
	exit_status = status_progress_finish("Loading AOW patch file", read, size);
	fflush(stdout);
	fclose(file);

	/* match pattern in data of file read */
	assert(sizeof pattern == sizeof replacement);
	for(read = 0; read < (size - sizeof pattern); read += sizeof pattern)
	{
		if (memcmp(&data[0] + read, pattern, sizeof pattern) == 0)
		{
			fprintf(stdout, "Pattern found %lu %lu\n", pattern_count, read);
			pattern_offsets[pattern_count++] = read;
		}
	}

	if(exit_status != EXIT_SUCCESS)
	{
		free(data);
		return exit_status;
	}

	for(read = 0; read < (size - sizeof pattern); read += sizeof pattern)
	{
		if (memcmp(&data[0] + read, replacement, sizeof replacement) == 0)
		{
			fprintf(stdout, "Replacement found %lu %lu\n", replacement_count, read);
			replacement_offsets[replacement_count++] = read;
		}
	}
	/* verify patch */
	if (pattern_count == 0 && replacement_count >= 2)
	{
		fprintf(stderr, "Patch already applied!\n");
		exit_status = EXIT_FAILURE;
	}
	if(exit_status != EXIT_SUCCESS)
	{
		free(data);
		return exit_status;
	}
	if (pattern_count < 2)
	{
		fprintf(stderr, "Patch not applicable!\n");
		exit_status = EXIT_FAILURE;
	}

	if(exit_status != EXIT_SUCCESS)
	{
		free(data);
		return exit_status;
	}

	/* replace patterns in data */
	size_t i = 0;
	for(i = 0; i < pattern_count; i++)
	{
		
		status_progress_update("Replacing pattern", i, pattern_count);
		memcpy(&data[pattern_offsets[i]], &replacement[0], sizeof replacement);
	}
	exit_status = status_progress_finish("Replacing pattern", i, pattern_count);

	if(exit_status != EXIT_SUCCESS)
	{
		free(data);
		return exit_status;
	}

	/* open file */
	file = fopen(filename, "wb");
	if(!file)
    	{
    		fprintf(stderr, "ERROR: Cannot open source file: %s!\n", filename);
		free(data);
    		exit(EXIT_FAILURE);
    	}

	/* write data in quad part chunks of 512 bytes and display progress */
	ptr = (file_block_t*)&data[0];
	for(written = 0; written < size; written += fwrite(ptr++, sizeof(uint8_t), sizeof(file_block_t), file))
		status_progress_update("Writing modified AOW patch file", written, size);
	exit_status = status_progress_finish("Writing modified AOW patch file", written, size);
	fflush(stdout);
	fflush(file);
	fclose(file);

	free(data);
	return exit_status == EXIT_FAILURE ? false : true;
}

