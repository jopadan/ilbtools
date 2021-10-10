#ifndef _PROGRESS_H
#define FILE_BLOCK_SIZE                  512

typedef quad_t file_block_t  __attribute__ ((__vector_size__(FILE_BLOCK_SIZE)));

/* rotate chars for progress status */
char *rotorchar = "-/|\\";

bool status_progress_update(const char* msg, size_t read, size_t size)
{
	static int rotor = 0;

	fprintf(stdout, "\r%s... %lu/%lu (%c)", msg, read, size, rotorchar[rotor]); 
	fflush(stdout);
	rotor++;
	rotor&=3;
	return (read == size);
}

int status_progress_finish(const char* msg, size_t read, size_t size)
{
    int exit_status = (read != size) ? EXIT_FAILURE : EXIT_SUCCESS;
    fprintf(stdout, "\r%s... %lu/%lu %s!\n", msg, read, size, exit_status == EXIT_FAILURE ? "FAILED" : "SUCCESS");
    fflush(stdout);
    return exit_status;
}

#endif
