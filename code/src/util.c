#include "util.h"

/**
 * Count the number of bits set in an integer.
 * @param x
 * @return Number of bits.
 */
int count_bits(int x) {
	int MASK = 1;
	int i;
	int sum = 0;
	for (i=0;i<32;i++) {
		if ((x&MASK) > 0) {
			sum = sum + 1;
		}
		MASK = MASK << 1;
	}
	return sum;
}


/**
 * Integer division with ceiling.
 * @param n Numerator
 * @param d Denominator
 * @return Result
 */
int ceil_div(int n, int d) {
	if(n%d == d || n%d == 0) {
		return n/d;
	} else {
		return n/d + 1;
	}
}

/**
 * Non-blocking terminal I/O: Check if the user pressed "Enter".
 * TODO: remove
 * @return 1 if button pressed.
 */
int input_available() {
  struct timeval tv;
  fd_set fds;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
  return (FD_ISSET(0, &fds));
}

// ------------------------------ MEMORY MAP/UNMAP ------------------------------ //

// map physical memory into virtual adress space
void* memory_map(unsigned int addr, size_t size) {
	// memory file
	int fd;
	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1){
		printf("Error: Failed to open memory");
		return NULL;
    }
	
	// map shared memory into userspace
	off_t target = addr;
	void* map_base = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
    if(map_base == (void *) -1) {
		printf("Error: Failed to map base address");
		return NULL;
    }
		
	close(fd);
	
	return map_base;
}

// unmap the mapped memory
int memory_unmap(void* ptr, size_t size) {
	if(munmap(ptr, size) == -1) {
		printf("Error: Failed to unmap memory");
		return -1;
    }
    
	return 1;
}

// ------------------------------ FILE READING/WRITING  ------------------------------ //

// reading integer from file
int read_file_value(char filename[]) {
	FILE* fp;
	unsigned int value = 0;
	fp = fopen(filename, "rt");
	if (fp < 0) {
		printf("Error: Cannot open file");
		return -1;
	}
	if(fscanf(fp, "%x", &value) < 0) {
		printf("Error: Cannot read from file");
		return -1;
	}
	fclose(fp);
	return value;
}


// ------------------------------ ERROR HANDLING ------------------------------ //

void rl_error(char message[]) {
	printf("Error: %s\n", message);
	
	// TODO: cleanup
	
	exit(EXIT_FAILURE);
}