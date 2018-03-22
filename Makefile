# depends on stormlib: https://github.com/ladislav-zezula/StormLib.git
# There is a small bug in the Linux makefile (Makefile.linux) in which
# ranlib -c fails, since there is no -c option. Just removing the -c
# seems to work.

# Don't forget to put the .so on your path:	export LD_LIBRARY_PATH=./StormLib

#depends on libjpeg: sudo apt-get install libjpeg-dev
# libsdl2-image-dev

SRC = main.c ll.c maps.c
CFLAGS = -IStormLib/src -LStormLib

all: $(SRC) libStorm.a
	gcc $(CFLAGS) -o war3c $(SRC) -lStorm -ljpeg

libStorm.a:
	make -C StormLib -f Makefile.linux -j4

tags:
	ctags -R -f tags *
