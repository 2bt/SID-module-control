#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <assert.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;

extern void c64Init(void);
extern word c64SidLoad(char *filename, word *init_addr, word *play_addr, byte *sub_song_start, byte *max_sub_songs, byte *speed, char *name, char *author, char *copyright);
extern void cpuJSR(word npc, byte na);

unsigned short init_addr;
unsigned short play_addr;
unsigned char song;
unsigned char max_songs;
unsigned char speed;

char name[32];
char author[32];
char copyright[32];


extern unsigned char memory[65536];

volatile int running = 1;
static void leave(int sig) { running = 0; }

static void flush(int serial) {
	assert(write(serial, "\xf7", 1) == 1);
}

static void play() {

	int serial = open("/dev/ttyUSB0", O_RDWR);
	struct termios config;
	tcgetattr(serial, &config);
	config.c_iflag = 0;
	config.c_oflag = 0;
	cfsetospeed(&config, B115200);
	cfsetispeed(&config, B115200);

	config.c_cflag = CS8|CREAD|CLOCAL;
	tcsetattr(serial, TCSANOW, &config);


	// zero out all registers
	int i;
	unsigned char t[2] = { 0, 0 };
	for(i = 0; i < 25; i++) {
		t[0] = i;
		assert(write(serial, t, 2) == 2);
	}
	flush(serial);

	unsigned char* reg = memory + 0xd400;
	unsigned char old_reg[25];

	while(running) {
		cpuJSR(play_addr, 0);


		for(i = 0; i < 25; i++) {
			if(old_reg[i] != reg[i])
			{
				t[0] = i;
				t[1] = reg[i];
				assert(write(serial, t, 2) == 2);
				old_reg[i] = reg[i];
			}
		}
		flush(serial);
		usleep(20000);
	}

	close(serial);
}


int main(int argc, char **argv) {

	if(argc < 2) {
		printf("usage: %s filename [songnumber]\n", argv[0]);
		return 0;
	}

	c64Init();
	int a = c64SidLoad(argv[1], &init_addr, &play_addr, &song, &max_songs,
						&speed, name, author, copyright);
	if(!a) {
		puts("error");
		return 1;
	}

	// song number
	if(argc > 2) {
		int s = atoi(argv[2]);
		if(s) {
			song = s - 1;
			if(song > max_songs) song = max_songs;
		}
	}

	printf("%s - %s - %s - %d/%d\n", name, author, copyright, song + 1, max_songs + 1);
	cpuJSR(init_addr, song);

	signal(SIGINT, leave);
	play();

	return 0;
}


