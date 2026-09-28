/* flexnet.h -- Linux NetPC server for Flex systems
	 Copyright (C) 2025 Michel Wurtz - mjwurtz@gmail.com

	 Server using the NetPC protocol used in Netpc35 (Bjarne Bäckstrom
	 and Ron Anderson). This software doesn't use any code from the
	 original program.

	 This program is free software; you can redistribute it and/or modify
	 it under the terms of the GNU General Public License as published by
	 the Free Software Foundation; either version 2, or (at your option)
	 any later version.

	 This program is distributed in the hope that it will be useful,
	 but WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 GNU General Public License for more details.

	 You should have received a copy of the GNU General Public License
	 along with this program; if not, write to the Free Software
	 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <getopt.h>
#include <dirent.h>
#include <stdarg.h>

// Sector size for Flex floppy
#define SECSIZE 256

#define LF  0x0a
#define CR  0x0d
#define ACK 0x06
#define NAK 0x15
#define ESC 0x1B

// Some global variables
char line[32];			// serial line to use (/dev/ttyS0, /dev/ttyUSB0, etc.)
int  speed = 0;			// serial line speed (19200 for Microbox on second ACIA)
FILE *serial;			// serial line handler

char param[128];		// Flexnet command parameters
int ready;				// Disk image ready ?
int readonly;			// Disk image readonly ?	
char curdir[256];		// Current directory

int exitOnLoadComplete = 0; // For testing only

static int verbose = 0;

//char filename[256];		// flex disk image path
//char *diskname;			// flex disk name
//int fd;					// file handler
uint8_t bloc[SECSIZE];	// current sector (for reading or writing)
//uint8_t nbtrk;			// nb of tracks on disk
//uint8_t nbsec;			// nb of sectors by track
//uint8_t track0l;		// nb of sectors on track 0

typedef struct {
    char filename[256];		// flex disk image path
    char *diskname;			// flex disk name
    int fd;					// file handler
//    uint8_t bloc[SECSIZE];	// current sector (for reading or writing)
    uint8_t nbtrk;			// nb of tracks on disk
    uint8_t nbsec;			// nb of sectors by track
    uint8_t track0l;		// nb of sectors on track 0
} imageFile_t;

imageFile_t imageFile[4];  // declare an array with four imageFile_t structs

int currentDrive = 0;       // specify the drive currently active

// forward declarations
void loop();
void msg(const char *fmt, ...);
