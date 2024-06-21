#include <stddef.h>
#include <stdint.h>
#include "../vmiolib.h"


void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {
	
	const char* filename = "text.txt";

    int fd = open(filename, O_WRONLY);

    char buffer[627] = "The  make  utility will determine automatically which pieces of a large program need to be recompiled, and issue the commands to recompile them.  The manual\n"
        "describes the GNU implementation of make, which was written by Richard Stallman and Roland McGrath, and is currently maintained by Paul Smith.  Our examples\n"
        "show C programs, since they are very common, but you can use make with any programming language whose compiler can be run with a shell  command.   In  fact,\n"
        "make is not limited to programs.  You can use it to describe any task where some files must be updated automatically from others whenever the others change.";

    write(fd, buffer, 627);

    close(fd);

    fd = open(filename, O_RDWR);

    read(fd, buffer, 10);

    for ( int i = 0; i < 10; i++ ) {
        putchar(buffer[i]);
    }

    close(fd);

	exit();
}
