# $Id: Makefile,v 1.14 2004/11/06 22:46:21 schwarze Exp $
# File: Makefile
# Copyright (C) 2003-2004 Ingo Schwarze
# Copyright (C) 2002 Marko Schulz
# Author: Marko Schulz <harleen@gmx.de>
# This file is part of MERCATOR.
# See the file COPYING for the GNU General Public License.

# --- adjust the following paths to your needs ---

# define your operating system
OSTYPE = -DUNIX
#OSTYPE = -DWIN32

# CONFIGPATH should contain the graphics (minimal, micro)
CONFIGPATH = /home/donchaos/eressea/mercator/
#CONFIGPATH = /usr/usta/common/libdata/mercator

# INCLPATH should contain png.h, pngconf.h and zlib.h
INCLPATH = -I/usr/local/include/libpng

# LIBPATH should contain libpng.a and libz.a
# unless those are in the system library directories
LIBPATH  = -L/usr/local/lib

# the following compiler flags make sense for production binaries
USERFLAGS = -Wall -ansi -pedantic -O2 -DNDEBUG -g

# in case you want to debug the Mercator sources, try something like
#USERFLAGS =  -gdwarf+ -Wall -ansi -pedantic -DMALLOC_DEBUG

# in case you want *.ere files for the gotoline utility, try
#USERFLAGS = -Wall -ansi -pedantic -O2 -DNDEBUG -DGOTOLINE

# your GNU compiler collection binary, preferably version 2.95
CC = gcc

# --- do not change anything below this line ---
# --- unless you know what you are doing     ---

CFLAGS = $(USERFLAGS) $(INCLPATH) $(LIBPATH) \
         $(OSTYPE) -DCONFIGPATH=\"$(CONFIGPATH)\"

SOURCES=mercator.c map.c html.c \
        parse/parser.c parse/zauber.c parse/meldungen.c \
        parse/parteien.c parse/einheiten.c \
        parse/region.c image.c language.c

OBJECTS = $(SOURCES:%.c=%.o)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

all:	mercator

clean:
	-rm -f *~ *.o parse/*.o *~ mercator mercator.core core

mercator:	$(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -lpng -lz -lm -o $@
        
$(OBJECTS):	map.h mercator.h parse/parser.h
