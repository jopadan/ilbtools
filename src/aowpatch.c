#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

/* rotate chars for progress status */
char *rotorchar = "-/|\\";

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

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        fprintf(stderr, "Usage: %s  -  %s\n", argv[0], "[src]");
    	exit(EXIT_FAILURE);
    }

    char* filename = argv[1];

    struct stat sb;

    if(stat(filename, &sb) != 0)
    {
    	fprintf(stderr, "ERROR: File not found: %s!\n", filename);
    	exit(EXIT_FAILURE);
    }

    assert(sizeof pattern == sizeof replacement);

    FILE* file = fopen(filename, "rb");
    if(!file)
    {
    	fprintf(stderr, "ERROR: Cannot open file: %s!\n", filename);
    	exit(EXIT_FAILURE);
    }

    uint8_t* data = calloc(1, sb.st_size * sizeof(uint8_t));

    /* quad part access ptr to data */
    u_quad_t* ptr = (u_quad_t*)&data[0];

    size_t read = 0;
    int rotor = 0;
    for(read = 0; read < sb.st_size; read += fread(ptr++, sizeof(uint8_t), sizeof(u_quad_t), file))
    {
	    fprintf(stdout, "\rLoading AOW patch file... %lu/%lu (%c)", read, sb.st_size, rotorchar[rotor]); 
	    fflush(stdout);
	    rotor++;
	    rotor&=3;
    }

    if( read != sb.st_size)
    {
	    fprintf(stdout, "\rLoading AOW patch file... %lu/%lu FAILED!\n", read, sb.st_size);
	    exit_status = EXIT_FAILURE;
    }
    else
    {
    	fprintf(stdout, "\rLoading AOW patch file... %lu/%lu SUCCESS!\n", read, sb.st_size, rotorchar[rotor]);
    }

    fflush(stdout);
    fclose(file);
    free(data);

    exit(exit_status);
}
