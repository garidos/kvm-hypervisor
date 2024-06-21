CC = gcc
CFLAGS = -g -o
COBJFLAGS = -m64 -Wno-pointer-to-int-cast -ffreestanding -fno-pic -c -o

LD = ld
LDFLAGS = -T
LDFILE = ./guests/guest.ld

EXECUTABLE = hypervisor
GUESTSDIR = ./guests
SHARED_FILES = shared_text.txt shared_info.txt

all: $(EXECUTABLE)

$(EXECUTABLE): main.c hypervisor.c
	$(CC) $(CFLAGS) $@ $^

%.img: $(GUESTSDIR)/%.o vmiolib.o
	$(LD) $(LDFLAGS) $(LDFILE) $^ -o $@

$(GUESTSDIR)/%.o: $(GUESTSDIR)/%.c
	$(CC) $(COBJFLAGS) $@ $^

vmiolib.o: vmiolib.c
	$(CC) $(COBJFLAGS) $@ $^

# two vm's access make new local files, write some text to them, read 10B out of them and write that out
testA: $(EXECUTABLE) guestA1.img guestA2.img
	./$(EXECUTABLE) -p 2 -m 8 -g $(word 2,$^) $(word 3,$^)

# two vm's access shared files, first they write into them, and by doing that local copies are made in their file systems
# after that they read from another shared file and write that out 
testB: $(EXECUTABLE) guestB1.img guestB2.img
	./$(EXECUTABLE) -p 4 --memory 2 -g $(word 2,$^) $(word 3,$^) -f $(SHARED_FILES)

test: $(EXECUTABLE) guest.img
	./$(EXECUTABLE) --page 2 -m 4 --guest $(word 2,$^)

# remove created vm file systems
remove_fs:
	rm -f -r *_FS

clean:
	rm -f $(EXECUTABLE) $(GUESTSDIR)/*.o *.img ./vmiolib.o

.PHONY: clean all test test_read test_write
	