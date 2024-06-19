#include <stddef.h>
#include <stdint.h>
#include "../hypervisor.h"


int open(const char* filename, int flags);
int close(int fd);
ssize_t write(int fd, const void* buf, size_t count);
ssize_t read(int fd, const void* buf, size_t count);
off_t lseek(int fd, off_t offset, int whence);
void putchar(char c);
void exit();



void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {
	
	const char* filename = "text.txt";

    int fd = open(filename, O_WRONLY);

    char buffer[476] = "Normally you should call your makefile either makefile or Makefile.  (We recommend Makefile because it appears prominently near the beginning of a directory\n"
        "listing, right near other important files such as README.)  The first name checked, GNUmakefile, is not recommended for most makefiles.  You should use this\n"
        "name if you have a makefile that is specific to GNU make, and will not be understood by other versions of make.  If makefile is '-', the standard  input  is\n"
        "read.";

    write(fd, buffer, 476);

    close(fd);

    fd = open(filename, O_RDWR);

    read(fd, buffer, 10);

    for ( int i = 0; i < 10; i++ ) {
        putchar(buffer[i]);
    }

    close(fd);

	exit();
}
