/* flexnet.c -- Linux NetPC server for Flex systems
	 Copyright (C) 2025 Michel Wurtz - mjwurtz@gmail.com
	 
	 Modification for multi-disk, 2026 Fred Jan Kraan fjkraan@electrickery.nl

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

#include "flexnet.h"

// Help message
void usage( char *cmd) {
	fprintf(stderr, "Usage: %s [-h] => this help\n", cmd);
	fprintf(stderr, "       %s [-v] -d <device> -s <speed> -[0123] <disk_image>\n", cmd);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, " -d <device> : serial line to use\n");
	fprintf(stderr, " -s <speed> : baudrate to use\n");
	fprintf(stderr, " -v : print requests to the server and reply (debug)\n");
    fprintf(stderr, " -t : for testing only; exit(0) after loading complete\n");
    fprintf(stderr, " -0, -1, -2 or -3 <disk_image> : provide image name\n");
}

// Convert track/sector to bloc number on the disc image

int ts2blk( uint8_t ntrk, uint8_t nsec) {
	if (ntrk > imageFile[currentDrive].nbtrk || 
        nsec > imageFile[currentDrive].nbsec || 
        (nsec == 0 && ntrk != 0)) {
		return( -1);
	}
	if (ntrk == 0) {	    // track 0 is straitforward...
		if (nsec == 0) {	// but sector=0 is not an error but a special case...
			return 0;
		} else {
			return (nsec - 1);
		}
	} else {
		return imageFile[currentDrive].track0l + (ntrk - 1) * imageFile[currentDrive].nbsec + nsec - 1;
	}
}

// read 8+3 name and insert a dot if necessary
// return the number of char of the name
// dot = 0 for volume name (11 chars, no ext.)
// return the length of name, parameter name updated
// The name should should a legal Flex filename

int getname( uint8_t *pos, char *name, int dot) {
	int k = 0;
	for (int j=0; j<11; j++) {
		if (!isalnum(pos[j]) && pos[j] != '-' && pos[j] != '_' && pos[j] != 0xFF
				&& pos[j] != ' ' && pos[j] != '*' && pos[j] != '.' && pos[j] != 0)
			return -1;
		if (dot && pos[j] == ' ')
			return -1;
		if (pos[j] != 0 )
			name[k++] = pos[j];
		if (j == 7)	{
			if (pos[8] == 0)
				break;
			if (dot)
				name[k++] = '.';
		}
	}
	name[k] = 0;
	return k;
}

// Analyse the validity of the image disk to load

int load_dsk(char *name, int driveNo) {

	struct stat dsk_stat;
	int size;
	int nb_sectors;
	char label[16];
	int volnum;
	int last_trk_sec;	
	int freesec; 
    
    msg(" load_dsk: Drive: %d, %s\n", driveNo, name);

	strncpy(imageFile[driveNo].filename, name, 256);
	imageFile[driveNo].diskname = strrchr( imageFile[driveNo].filename, '/');
	if (imageFile[driveNo].diskname == NULL)
		imageFile[driveNo].diskname = imageFile[driveNo].filename;
	else
		imageFile[driveNo].diskname++;

	if (stat( imageFile[driveNo].filename, &dsk_stat)) {
		if (verbose)
			perror( imageFile[driveNo].filename); 
		return -1;
	}

	// Open disk image
	imageFile[driveNo].diskname = strrchr( imageFile[driveNo].filename, '/');
	if (imageFile[driveNo].diskname == NULL)
		imageFile[driveNo].diskname = imageFile[driveNo].filename;
	else
		imageFile[driveNo].diskname++;
	size = dsk_stat.st_size;
	if (dsk_stat.st_mode & S_IWUSR) {
		if ((imageFile[driveNo].fd = open( imageFile[driveNo].filename, O_RDWR)) < 0) {
			if (verbose)
				perror(imageFile[driveNo].diskname);
			return -1;
		}
		readonly = 0;
	} else {
		if ((imageFile[driveNo].fd = open( imageFile[driveNo].filename, O_RDONLY)) < 0 ) {
			if (verbose)
				perror( imageFile[driveNo].diskname);
			return -1;
		}
		readonly = 1;
	}
    // Load track 0, sector 2, the SIR
	lseek( imageFile[driveNo].fd, SECSIZE*2, SEEK_SET);
	if (read( imageFile[driveNo].fd, bloc, SECSIZE) != SECSIZE)
		return -1;

	nb_sectors = size / SECSIZE;
	if (nb_sectors * SECSIZE != size) {
		fprintf( stderr, "Disk size don't match an integer number of sectors: %u bytes left]\n",
			size % SECSIZE);
		return -1;
	}

	msg( "Opening %s (%u sectors)\n", imageFile[driveNo].diskname, nb_sectors);

	// Not a flex disk ?
	if (getname( bloc + 0x10, label, 0) < 0 || 
                bloc[0x26] == 0 ||
                bloc[0x27] == 0) {
		fprintf( stderr, "Not a valid Flex disk image: ");
		return -1;
	}

	volnum = bloc[0x1b]*256 + bloc[0x1c];
	// Size of disk & free sector list
	imageFile[driveNo].nbtrk = bloc[0x26];
	imageFile[driveNo].nbsec = bloc[0x27];
	freesec = bloc[0x21]*256 + bloc[0x22];

	// Too much free sectors for the disk ?
	if (freesec > imageFile[driveNo].nbtrk * imageFile[driveNo].nbsec && 
        verbose)
		msg("Warning: Number of free sectors bigger than disk size\n");

	// Print info about the disk
	msg("Flex Volume name: '%s', volume number %d (%d tracks, %d sectors/track)\n",
			label, volnum, imageFile[driveNo].nbtrk+1, imageFile[driveNo].nbsec);

	// Try to guess disk geometry
	if ((imageFile[driveNo].nbtrk+1) * imageFile[driveNo].nbsec == nb_sectors) {
		msg("Looks like a Single Density disk\n");
		imageFile[driveNo].track0l = imageFile[driveNo].nbsec;
	} else {
		imageFile[driveNo].track0l = nb_sectors - imageFile[driveNo].nbtrk * imageFile[driveNo].nbsec;
		if ((imageFile[driveNo].nbsec >= 36 && imageFile[driveNo].track0l == 20) ||
			(imageFile[driveNo].nbsec == 18 && imageFile[driveNo].track0l == 10) ||
			(imageFile[driveNo].track0l == imageFile[driveNo].nbsec/2)) {
			msg("Looks like a Double Density disk with Single Density track 0 of %d sectors\n",
	    			imageFile[driveNo].track0l);
		} else if (imageFile[driveNo].track0l > imageFile[driveNo].nbsec) {
			// Weird geometry... but can happen when disks are in EEPROM
			msg("Unknown geometry: %d tracks of %d sectors + first track of %d sectors !\n",
	    			imageFile[driveNo].nbtrk, 
                    imageFile[driveNo].nbsec, 
                    imageFile[driveNo].track0l);
			imageFile[driveNo].track0l = imageFile[driveNo].nbsec;
			imageFile[driveNo].nbtrk++;
			last_trk_sec = nb_sectors - (imageFile[driveNo].nbtrk-1) * imageFile[driveNo].nbsec - imageFile[driveNo].track0l;
			msg(" => Using normal %d sector track 0, add a %d%s incomplete track of %d sectors\n",
	    			imageFile[driveNo].track0l, 
                    imageFile[driveNo].nbtrk, "th", 
                    last_trk_sec);
		} else if (imageFile[driveNo].track0l > imageFile[driveNo].nbsec/2 && 
                    imageFile[driveNo].track0l < imageFile[driveNo].nbsec) {
			msg("Looks like a Double Density disk with Single Density track 0 of %d sectors\n",
	    			imageFile[driveNo].track0l);
		} else {
			imageFile[driveNo].nbtrk -= 
                (((imageFile[driveNo].nbtrk * imageFile[driveNo].nbsec - nb_sectors) / imageFile[driveNo].nbsec) + 1);
			// This is generaly no good, trying to guess end of track 0
			fprintf(stderr, "ERROR: Disk image too small... unusual geometry or truncated ?\n");
			return -1;
		}
	}
	ready = 1;
	return 0;
}

// get the parameters of a command

void getparam() {
	int c, i;
	i = 0;
	
	while ((c = fgetc( serial)) != CR) {
		if (i<127)
			param[i++] = c;
		else
			param[i] = 0;
	}
	param[i] = 0;
}

// Checksum for disk bloc transfer

int checksum( uint8_t *data) {
	int chks;

	chks = 0;
	for (int i = 0; i < 256; i++)
		chks += (unsigned int) data[i];
	return chks & 0xFFFF;
}

// S command : Read a sector on disk image and send it

void sndblk() {
	int drv, msb, lsb, chks;		// For checksum computing and transmitting
	int retval;
	int pos;
	int nsec, ntrk;

	drv = fgetc( serial);
	ntrk = fgetc( serial);
	nsec = fgetc( serial);
	retval = 1;
    currentDrive = drv;

	if (ready == 0) {		// force checksum error if disk not ready
		msg("No disk mounted, force CRC error!\n");
		for (int i = 0; i < 258; i++)
			fputc( 0, serial);
		fputc( 1, serial);
		if ((retval = fgetc( serial)) != NAK)
            msg("... unexpected return value : 0x%02X\n", retval);
		return ;
	}

	if ((pos = SECSIZE * ts2blk( ntrk, nsec)) < 0) {
		retval = 0;
	} else {
		if (lseek( imageFile[currentDrive].fd, pos, SEEK_SET) != pos)
			retval = 0;
		if (read( imageFile[currentDrive].fd, bloc, SECSIZE) != SECSIZE)
			retval = 0;
	}
	if (retval == 0) {
		for( int i = 0; i< 256; i++) 
			bloc[i] = 0;
    }
    if (retval) {
        msg("Bloc dsk %d [0x%02X/0x%02X] (pos = %d) read", 
            drv, ntrk, nsec, pos);
    } else {
        msg("Fail to read bloc dsk %d [0x%02X/0x%02X] (pos = %d)", 
            drv, ntrk, nsec, pos);
    }

	chks = checksum( bloc);
	lsb = chks & 0xFF;
	msb = (chks >> 8) & 0xFF;
	for( int i = 0; i< 256; i++)
		fputc( bloc[i], serial);
	fputc( msb, serial);
	fputc( lsb, serial);

	retval = fgetc( serial);
	if (verbose) {
		if (retval == NAK)
			msg("... transmission failed\n");
		else if (retval == ACK)
			msg("... transmission OK\n");
		else
			msg("... return value not expected : 0x%02X\n", retval);
    }
}

// Command R : Receive a disk sector and write in on disk image

int rcvblk() {
	int msb, lsb, chks;		// For checksum computing and transmitting
	int retval;
	int pos;
	int i;

	int drv = fgetc( serial);
	int ntrk = fgetc( serial);
	int nsec = fgetc( serial);
	pos = SECSIZE * ts2blk( ntrk, nsec);
    currentDrive = drv;

	for (i = 0; i <256; i++)
		bloc[i] = fgetc( serial);
	msb = fgetc( serial);
	lsb = fgetc( serial);
	retval = 1;

	if ((chks = checksum( bloc)) == msb * 256 + lsb) {
		if (pos < 0)
			retval = 0;
		else {
			if (ready == 0)
				return (retval = 0);
			if (lseek( imageFile[currentDrive].fd, pos, SEEK_SET) != pos)
				retval = 0;
			if (write( imageFile[currentDrive].fd, bloc, SECSIZE) != SECSIZE)
				retval = 0;
		}
	} else {
		retval = 0;
		if (verbose) {
			msg( "Bad checksum (0x%04X instead of 0x%04X)\n", 
                msb * 256 + lsb, chks);
			for (i = 0; i< 256; i++)
				printf ("%c0x%02x", i%16?' ':'\n', bloc[i]);
		}
	}
	if (verbose) {
		if (retval)
			msg( "Bloc dsk %d [0x%02X/0x%02X] (pos = %d) written\n", 
                drv, ntrk, nsec, pos);
		else
			msg( "Fail to write bloc dsk %d [0x%02X/0x%02X] (pos = %d)\n",
                drv, ntrk, nsec, pos);
    }
	return retval;
}

// RCD command

int chngd() {
	int retval = 0;
	if (chdir( param) < 0) {
		retval = 0;
		msg( "Cannot change directory to %s\n", param);
	} else {
		getcwd( curdir, 255);
		retval = 1;
		msg( "Changing directory to %s\n", curdir);
	}
	return retval;
}

// RMOUNT command

int rmount(int drive) {
	char rfilename[256];

    if (imageFile[currentDrive].fd != -1) {
        close(imageFile[currentDrive].fd);
        msg("closing %d: %s\n", drive, imageFile[currentDrive].diskname);
    }

	ready = 1;
	strncpy(rfilename, param, 255);
	strncat(rfilename, ".DSK", 255);	// Rmount don't put the extension
	if (load_dsk(rfilename, drive) < 0) {
		msg("trying with lowercase...\n");
		strncpy(rfilename, param, 255);
		strncat(rfilename, ".dsk", 255);
		if (load_dsk(rfilename, drive) < 0)
			ready = 0;
	}
	return ready;
}

// RDIR command

void lstdsk() {
	struct dirent *entry;
	DIR *dirp;
	int reply;
	int endlist;

	getparam();
	
	msg("RDIR(%s) command\n", param);
				
	fputc(CR, serial);
	fputc(LF, serial);

	dirp = opendir(curdir);
	endlist = 1;
	while ((entry = readdir(dirp)) != NULL) {
		if (strcasecmp((entry->d_name)+strlen(entry->d_name)-3, "DSK") != 0) 
			continue;
		if (strcasestr(entry->d_name, param) != entry->d_name)
			continue;
		if ((reply = fgetc( serial)) != ' ') {
			if (reply != ESC)
				msg("Unexpected command (0x%02X) while reading directory\n", 
                    reply);
			endlist = 0;
			break;
		}
		msg("---> %s\n", entry->d_name);
		fputs(entry->d_name, serial);
		fputc(CR, serial);
		fputc(LF, serial);
	}
	if (endlist)
		if ((reply = fgetc( serial)) != ' ')
			msg("Unexpected command (0x%02X) while reading directory\n", 
                    reply);

	closedir(dirp);
	fputc(ACK, serial);
}

// RLIST command

void lstdir() {
	struct dirent *entry;
	struct stat statbuf;
	DIR *dirp;
	int reply;
	int endlist;

	msg("RLIST command\n");
				
	getparam();
	if ((reply = fgetc( serial)) != 0x20)
		printf("Bad char 0x%02X received...\n", reply);
	else {
		fputc(CR, serial);
		fputc(LF, serial);
	}

	endlist = 1;
	dirp = opendir(curdir);
	while ((entry = readdir( dirp)) != NULL) {
		if (strcmp(entry->d_name, ".") * strcmp(entry->d_name, "..") == 0)
			continue;
		if (stat(entry->d_name, &statbuf) == -1) {
			if (verbose)
				perror(entry->d_name);
			continue;
		}
		if (S_ISDIR(statbuf.st_mode) == 0) 
			continue;
		if ((reply = fgetc( serial)) != 0x20) {
			if (reply != ESC)
				msg("Unexpected command (0x%02X) while reading directory\n", 
                    reply);
			endlist = 0;
			break;
		}
		msg("---> %s\n", entry->d_name);
		fputs(entry->d_name, serial);
		fputc(CR, serial);
		fputc(LF, serial);
	}
	if (endlist)
		if ((reply = fgetc( serial)) != ' ')
			msg("Unexpected command (0x%02X) while reading directory\n", 
                    reply);
	closedir(dirp);
	fputc(ACK, serial);
}

// Program starts here

int main( int argc, char **argv)
{
	int opt;
//	char *name;
//	struct stat dsk_stat;
//	int flags;
	struct termios linespec;
	int idlnk;
    
    // Set initial state of disk images
    imageFile[0].fd = -1;
    imageFile[1].fd = -1;
    imageFile[2].fd = -1;
    imageFile[3].fd = -1;

// Read parameters
	while ((opt = getopt( argc, argv, "d:s:0:1:2:3:vht")) != -1) {
		switch (opt) {
			case 'h':
				usage( *argv);
				exit( 0);
				break;
			case 'v':
				verbose = 1;
				break;
			case 'd':
				strncpy(line, optarg, 31) ;
				break;
			case 's':
				sscanf(optarg, "%d", &speed);
				break;
            case '0':
                load_dsk(optarg, 0);
                break;
            case '1':
                load_dsk(optarg, 1);
                break;
            case '2':
                load_dsk(optarg, 2);
                break;
            case '3':
                load_dsk(optarg, 3);
                break;
			case 't':
				exitOnLoadComplete = 1;
				break;
			default: /* unknown commands */
				usage( *argv);
				exit(1);
		}
	}

// Some sanitary checking on options
	if (strlen(line) == 0) {
		fprintf(stderr, "No serial line ?\n");
		usage(*argv);
		exit(1);
	}

	if (speed == 0) {
		fprintf(stderr, "No baudrate ?\n");
		usage(*argv);
		exit(1);
	}

	if((serial = fopen(line, "r+")) == NULL )
		perror(line);

	if ((idlnk = fileno(serial)) < 0)
			perror(line);

	if (tcgetattr(idlnk, &linespec) < 0) {
		perror("ERROR getting current terminal's attributes");
		exit(1);
	}
	cfmakeraw(&linespec);
	cfsetspeed(&linespec, speed);
		
	if (tcsetattr(idlnk, TCSANOW, &linespec) < 0) {
		perror ("ERROR setting current terminal's attributes");
		exit(1);
	}

	msg( "Link on %s, speed is %d bauds\n", line, speed);
    
    int d = 0;
    printf(" Mounted: fd: %d, Drive %d arg: '%s', diskname: '%s'\n", 
        imageFile[d].fd, d, imageFile[d].filename, imageFile[d].diskname);
    d = 1;
    if (imageFile[d].fd != -1) {
        printf(" Mounted: fd: %d, Drive %d arg: '%s', diskname: '%s'\n", 
            imageFile[d].fd, d, imageFile[d].filename, imageFile[d].diskname);
    }
    d = 2;
    if (imageFile[d].fd != -1) {
        printf(" Mounted: fd: %d, Drive %d arg: '%s', diskname: '%s'\n", 
            imageFile[d].fd, d, imageFile[d].filename, imageFile[d].diskname);
    }
    d = 3;
    if (imageFile[d].fd != -1) {
        printf(" Mounted: fd: %d, Drive %d arg: '%s', diskname: '%s'\n", 
            imageFile[d].fd, d, imageFile[d].filename, imageFile[d].diskname);
    }
    
    // Testing option for the make tests run
    if (exitOnLoadComplete) {
        exit(0);
    }

    loop();
}

void loop() {
	int command;

	// Read command from flex side
	
	while (1) {
		command = fgetc(serial);
		*param = 0;
		switch (command) {
			case 0x55:
			case 0xAA:
				fputc(command, serial);
				msg( "Initial sync or RESYNC command ($%02x)\n", command);
				break;
			case 'S':	// send a block 
			case 's':	// FLEXNET use lower case
				sndblk();
				break;
			case 'R':	// receive a bloc
			case 'r':	// FLEXNET use lower case
				fputc(rcvblk() ? ACK : NAK, serial);
				break;
			case 'V':	// Query MS-DOS drive letter - no use for Unix ;-)
				getparam();
				fputc(ACK, serial);
				msg( "Query (change) drive command\n");
				break;
			case '?':	// 
				fputs(curdir, serial);
				fputc(CR, serial);
				fputc(ACK, serial);
				msg( "Query current directory (%s) command\n", param);
				break;
			case 'Q':
				fputc(ACK, serial);
				msg("Quick check: is drive ready ? (unix: allways yes)\n");
				break;
			case 'A':	// list .dsk files
				lstdsk();
				break;
			case 'I':	// list subdirectories (not yet implemented)
				lstdir();
				break;
			case 'C':	// create .dsk file (not yet implemented)
				getparam();
				getparam();
				getparam();
				getparam();
                break;
			case 'D':	// delete .dsk file (not yet implemented)
				getparam();
				fputc(NAK, serial);
				msg("%s(%s) command (no action, reply NAK)\n",
						command=='C' ? "RCREATE" : "RDELETE", param);
				break;
			case 'E':	// Flex leave
				fputc (ACK, serial);
				msg("Flexnet exit\n");
				exit(0);
			case 'P':	// change directory -- param = path
				getparam();
				fputc(chngd() ? ACK : NAK, serial);
				break;
			case 'M':	// mount a new disk image
				getparam();
				if (rmount(currentDrive)) {
					fputc(ACK, serial);
					fputc(readonly ? 'R' : 'W', serial);
				} else
					fputc(NAK, serial);
				break;
			case -1:
				fprintf(stderr, "Serial line disappeared - Panic exit\n");
				exit( 1);
			default:	// WTF ? Something wrong happened... Just ignore...
				printf("Unknown command 0x%02x (%c)\n", 
                        command, isprint( command) ? command : 0);
				break;
		}
	}
}

void msg(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
}

