SRC = src/player.c src/6502.c

player: $(SRC)
	gcc -Wall -O2 -o $@ $(SRC)

