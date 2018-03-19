# depends on stormlib: https://github.com/ladislav-zezula/StormLib.git
# There is a small bug in the Linux makefile (Makefile.linux) in which
# ranlib -c fails, since there is no -c option. Just removing the -c
# seems to work.

#depends on libjpeg: sudo apt-get install libjpeg-dev
# libsdl2-image-dev

SRC = main.c ll.c maps.c
CFLAGS = -I../StormLib/src -L../StormLib

all: $(SRC)
	gcc $(CFLAGS) -o war3c $(SRC) -lStorm -ljpeg
