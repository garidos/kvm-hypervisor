#ifndef VMIOLIB_H
#define VMIOLIB_H

#include <stdint.h>

#ifndef O_RDONLY
# define O_RDONLY   00
#endif
#ifndef O_WRONLY
# define O_WRONLY   01
#endif
#ifndef O_RDWR
# define O_RDWR     02
#endif
#ifndef O_CREAT
# define O_CREAT    0100
#endif
#ifndef O_TRUNC
# define O_TRUNC    01000	
#endif
#ifndef O_APPEND
# define O_APPEND   02000
#endif

#ifndef SEEK_SET
# define SEEK_SET	0	
#endif
#ifndef SEEK_CUR
# define SEEK_CUR	1
#endif
#ifndef SEEK_END	
# define SEEK_END	2
#endif

#define OPEN    1
#define CLOSE   2
#define READ    3
#define WRITE   4
#define SEEK    5

#if !defined(ssize_t)
    typedef long ssize_t;
#endif
#if !defined(size_t)
    typedef long unsigned int size_t;
#endif
#if !defined(off_t)
    typedef long off_t;
#endif

static const uint16_t file_port = 0x278;
static const uint16_t terminal_port = 0xe9;

struct io_open {
    const char* filename;
    int flags;
};

struct io_rw {
    const void* buffer;
    int n;
};

struct io_seek {
    off_t offset;
    int whence;
};

struct io_struct {
    union {
        struct  io_open io_open;
        struct  io_rw io_rw;
        struct  io_seek io_seek;
    };
    uint8_t type;  
    int fd;
    uint64_t ret;
};

#ifndef _STDIO_H

int open(const char* filename, int flags); 
int close(int fd);
ssize_t write(int fd, const void* buf, size_t count);
ssize_t read(int fd, const void* buf, size_t count);
off_t lseek(int fd, off_t offset, int whence);

void putchar(char c);
char getchar();
void print_string(const char* str, int n);
void print_nt_string(const char* str);

void exit();

#endif


#endif