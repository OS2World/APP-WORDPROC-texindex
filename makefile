#Let's see how far just compiling the sources will get us into making a wokring
#TeXIndex program for OS/2, using the UNIX sources
#
#cmi dec '94

CFLAGS = -O -DSTDC_HEADERS
CC = gcc
LDFLAGS = -static

SOURCES = info.c makeinfo.c texindex.c getopt.c getopt1.c
OBJECTS = info.obj makeinfo.obj texindex.obj getopt.obj getopt1.obj
HEADERS = getopt.h

PROGS = info.exe makeinfo.exe texindex.exe

#texindex_pp.exe: texindex_pp.obj getopt.obj getopt1.obj
#    $(CC) $(LDFLAGS) -o $@ texindex_pp.obj getopt.obj getopt1.obj

texindex.exe:   texindex.obj getopt.obj getopt1.obj
    $(CC) $(LDFLAGS) -o $@ texindex.obj getopt.obj getopt1.obj

info.exe:   info.obj getopt.obj getopt1.obj
    $(CC) $(LDFLAGS) -o $@ info.obj getopt.obj getopt1.obj

info.obj:   info.c getopt.h
    $(CC) -c $(CFLAGS) info.c

makeinfo.exe:   makeinfo.obj getopt.o getopt1.obj
    $(CC) $(LDFLAGS) -o $@ makeinfo.obj getopt.obj getopt1.obj

makeinfo.obj:   makeinfo.c getopt.h
    $(CC) -c $(CFLAGS) makeinfo.c

.c.obj:
    $(CC) -c $(CFLAGS) $<

#info.obj makeinfo.obj texindex.obj getopt1.obj: getopt.h
