# depends on stormlib: https://github.com/ladislav-zezula/StormLib.git
# There is a small bug in the Linux makefile (Makefile.linux) in which
# ranlib -c fails, since there is no -c option. Just removing the -c
# seems to work.

# dependencies:
# sudo apt-get install libbz2-dev
# Don't forget to put the .so on your path:	export LD_LIBRARY_PATH=./StormLib

#depends on libjpeg: sudo apt-get install libjpeg-dev
# libsdl2-image-dev

SRC = main.c ll.c maps.c ka_array.c
CFLAGS = -IStormLib/src -LStormLib

all: $(SRC) libStorm.a
	gcc $(CFLAGS) -o war3c $(SRC) -lStorm -lrt -lpthread

libStorm.a:
	make -C StormLib -f Makefile.linux -j4

clean:
	make -C StormLib -f Makefile.linux clean
	rm -rf *.o
	rm -rf war3c

tags:
	ctags -R -f tags *
