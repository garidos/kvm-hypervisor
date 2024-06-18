CC = gcc
CFLAGS = -g -o
COBJFLAGS = -m64 -Wno-pointer-to-int-cast -ffreestanding -fno-pic -c -o

LD = ld
LDFLAGS = -T
LDFILE = ./guests/guest.ld

EXECUTABLE = hypervisor

GUESTSDIR = ./guests

all: $(EXECUTABLE)

$(EXECUTABLE): main.c hypervisor.c
	$(CC) $(CFLAGS) $@ $^

%.img: $(GUESTSDIR)/%.o functions.o
	$(LD) $(LDFLAGS) $(LDFILE) $^ -o $@

$(GUESTSDIR)/%.o: $(GUESTSDIR)/%.c
	$(CC) $(COBJFLAGS) $@ $^

functions.o: functions.c
	$(CC) $(COBJFLAGS) $@ $^

test_read: $(EXECUTABLE) file_rd_guest1.img file_rd_guest2.img
	./$(EXECUTABLE) -p 2 -m 8 -g $(word 2,$^) $(word 3,$^) -f ./files/test.txt

test_write: $(EXECUTABLE) file_wr_guest1.img file_wr_guest2.img
	./$(EXECUTABLE) -p 4 -m 2 -g $(word 2,$^) $(word 3,$^) -f ./files/text.txt

test: $(EXECUTABLE) guest.img
	./$(EXECUTABLE) -p 2 -m 4 -g $(word 2,$^)



clean:
	rm -f $(EXECUTABLE) $(GUESTSDIR)/*.o *.img ./functions.o

.PHONY: clean all test test_read test_write
	