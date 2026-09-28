/* flexnet.c -- Linux NetPC server for Flex systems
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

#include "flexnet.h"

// Help message
void usage( char *cmd) {
	fprintf( stderr, "Usage: %s [-h] => this help\n", cmd);
	fprintf( stderr, "       %s [-v] -d <device> -s <speed> disk_image\n", cmd);
	fprintf( stderr, "Options:\n");
	fprintf( stderr, " -d <device> : serial line to use\n");
	fprintf( stderr, " -s <speed> : baudrate to use\n");
	fprintf( stderr, " -v : print requests to the server and reply (debug)\n");
}

// Convert track/sector to bloc number on the disc image

int ts2blk( uint8_t ntrk, uint8_t nsec) {
	if (ntrk > imageFile[0].nbtrk || 
        nsec > imageFile[0].nbsec || 
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
		return imageFile[0].track0l + (ntrk - 1) * imageFile[0].nbsec + nsec - 1;
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

int load_dsk( char *name) {

	struct stat dsk_stat;
	int size;
	int nb_sectors;
	char label[16];
	int volnum;
	int last_trk_sec;	
	int freesec; 

	strncpy( imageFile[0].filename, name, 256);
	imageFile[0].diskname = strrchr( imageFile[0].filename, '/');
	if (imageFile[0].diskname == NULL)
		imageFile[0].diskname = imageFile[0].filename;
	else
		imageFile[0].diskname++;

	if (stat( imageFile[0].filename, &dsk_stat)) {
		if (verbose)
			perror( imageFile[0].filename); 
		return -1;
	}

	// Open disk image
	imageFile[0].diskname = strrchr( imageFile[0].filename, '/');
	if (imageFile[0].diskname == NULL)
		imageFile[0].diskname = imageFile[0].filename;
	else
		imageFile[0].diskname++;
	size = dsk_stat.st_size;
	if (dsk_stat.st_mode & S_IWUSR) {
		if ((imageFile[0].fd = open( imageFile[0].filename, O_RDWR)) < 0) {
			if (verbose)
				perror( imageFile[0].diskname);
			return -1;
		}
		readonly = 0;
	} else {
		if ((imageFile[0].fd = open( imageFile[0].filename, O_RDONLY)) < 0 ) {
			if (verbose)
				perror( imageFile[0].diskname);
			return -1;
		}
		readonly = 1;
	}
	lseek( imageFile[0].fd, SECSIZE*2, SEEK_SET);
	if (read( imageFile[0].fd, imageFile[0].bloc, SECSIZE) != SECSIZE)
		return -1;

	nb_sectors = size / SECSIZE;
	if (nb_sectors * SECSIZE != size) {
		fprintf( stderr, "Disk size don't match an integer number of sectors: %u bytes left]\n",
			size % SECSIZE);
		return -1;
	}

	if (verbose)
		printf( "Opening %s (%u sectors)\n", imageFile[0].diskname, nb_sectors);

	// Not a flex disk ?
	if (getname( imageFile[0].bloc + 0x10, label, 0) < 0 || 
                imageFile[0].bloc[0x26] == 0 ||
                imageFile[0].bloc[0x27] == 0) {
		fprintf( stderr, "Not a valid Flex disk image: ");
		return -1;
	}

	volnum = imageFile[0].bloc[0x1b]*256 + imageFile[0].bloc[0x1c];
	// Size of disk & free sector list
	imageFile[0].nbtrk = imageFile[0].bloc[0x26];
	imageFile[0].nbsec = imageFile[0].bloc[0x27];
	freesec = imageFile[0].bloc[0x21]*256 + imageFile[0].bloc[0x22];

	// Too much free sectors for the disk ?
	if (freesec > imageFile[0].nbtrk * imageFile[0].nbsec && verbose)
		printf( "Warning: Number of free sectors bigger than disk size\n");

	// Print info about the disk
	if (verbose)
		printf( "Flex Volume name: '%s', volume number %d (%d tracks, %d sectors/track)\n",
			label, volnum, imageFile[0].nbtrk+1, imageFile[0].nbsec);

	// Try to guess disk geometry
	if ((imageFile[0].nbtrk+1) * imageFile[0].nbsec == nb_sectors) {
		if (verbose) {
			printf( "Looks like a Single Density disk\n");
		}
		imageFile[0].track0l = imageFile[0].nbsec;
	} else {
		imageFile[0].track0l = nb_sectors - imageFile[0].nbtrk * imageFile[0].nbsec;
		if ((imageFile[0].nbsec >= 36 && imageFile[0].track0l == 20) ||
			(imageFile[0].nbsec == 18 && imageFile[0].track0l == 10) ||
			(imageFile[0].track0l == imageFile[0].nbsec/2)) {
			if (verbose)
	  			printf ( "Looks like a Double Density disk with Single Density track 0 of %d sectors\n",
	    			imageFile[0].track0l);
		} else if (imageFile[0].track0l > imageFile[0].nbsec) {
			// Weird geometry... but can happen when disks are in EEPROM
			if (verbose)
	  			printf( "Unknown geometry: %d tracks of %d sectors + first track of %d sectors !\n",
	    			imageFile[0].nbtrk, imageFile[0].nbsec, imageFile[0].track0l);
			imageFile[0].track0l = imageFile[0].nbsec;
			imageFile[0].nbtrk++;
			last_trk_sec = nb_sectors - (imageFile[0].nbtrk-1) * imageFile[0].nbsec - imageFile[0].track0l;
			if (verbose)    
	  			printf( " => Using normal %d sector track 0, add a %d%s incomplete track of %d sectors\n",
	    			imageFile[0].track0l, imageFile[0].nbtrk, "th", last_trk_sec);
		} else if (imageFile[0].track0l > imageFile[0].nbsec/2 && imageFile[0].track0l < imageFile[0].nbsec) {
			if(verbose)
	  			printf ( "Looks like a Double Density disk with Single Density track 0 of %d sectors\n",
	    			imageFile[0].track0l);
		} else {
			imageFile[0].nbtrk -= (((imageFile[0].nbtrk * imageFile[0].nbsec - nb_sectors) / imageFile[0].nbsec) + 1);
			// This is generaly no good, trying to guess end of track 0
			fprintf( stderr, "ERROR: Disk image too small... unusual geometry or truncated ?\n");
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
	uint8_t nsec, ntrk;

	drv = fgetc( serial);
	ntrk = fgetc( serial);
	nsec = fgetc( serial);
	retval = 1;

	if (ready == 0) {		// force checksum error if disk not ready
		if (verbose)
			printf( "No disk mounted, force CRC error!\n");
		for (int i = 0; i < 258; i++)
			fputc( 0, serial);
		fputc( 1, serial);
		if ((retval = fgetc( serial)) != NAK && verbose)
				printf ("... unexpected return value : 0x%02X\n", retval);
		return ;
	}

	if ((pos = SECSIZE * ts2blk( ntrk, nsec)) < 0) {
		retval = 0;
	} else {
		if (lseek( imageFile[0].fd, pos, SEEK_SET) != pos)
			retval = 0;
		if (read( imageFile[0].fd, imageFile[0].bloc, SECSIZE) != SECSIZE)
			retval = 0;
	}
	if (retval == 0) {
		for( int i = 0; i< 256; i++) 
			imageFile[0].bloc[i] = 0;
    }
	if (verbose) {
		if (retval) 
			printf( "Bloc dsk %d [0x%02X/0x%02X] (pos = %d) read", drv, ntrk, nsec, pos);
		else
			printf( "Fail to read bloc dsk %d [0x%02X/0x%02X] (pos = %d)", drv, ntrk, nsec, pos);
    }
	chks = checksum( imageFile[0].bloc);
	lsb = chks & 0xFF;
	msb = (chks >> 8) & 0xFF;
	for( int i = 0; i< 256; i++)
		fputc( imageFile[0].bloc[i], serial);
	fputc( msb, serial);
	fputc( lsb, serial);

	retval = fgetc( serial);
	if (verbose) {
		if (retval == NAK)
			printf( "... transmission failed\n");
		else if (retval == ACK)
			printf ("... transmission OK\n");
		else
			printf ("... return value not expected : 0x%02X\n", retval);
    }
}

// Command R : Receive a disk sector and write in on disk image

int rcvblk() {
	int msb, lsb, chks;		// For checksum computing and transmitting
	int retval;
	int pos;
	uint8_t nsec, ntrk;
	int i;

//	int drv = fgetc( serial);
	ntrk = fgetc( serial);
	nsec = fgetc( serial);
	pos = SECSIZE * ts2blk( ntrk, nsec);

	for (i = 0; i <256; i++)
		imageFile[0].bloc[i] = fgetc( serial);
	msb = fgetc( serial);
	lsb = fgetc( serial);
	retval = 1;

	if ((chks = checksum( imageFile[0].bloc)) == msb * 256 + lsb) {
		if (pos < 0)
			retval = 0;
		else {
			if (ready == 0)
				return (retval = 0);
			if (lseek( imageFile[0].fd, pos, SEEK_SET) != pos)
				retval = 0;
			if (write( imageFile[0].fd, imageFile[0].bloc, SECSIZE) != SECSIZE)
				retval = 0;
		}
	} else {
		retval = 0;
		if (verbose) {
			printf( "Bad checksum (0x%04X instead of 0x%04X)\n", msb * 256 + lsb, chks);
			for (i = 0; i< 256; i++)
				printf ("%c0x%02x", i%16?' ':'\n', imageFile[0].bloc[i]);
		}
	}
	if (verbose) {
		if (retval)
			printf( "Bloc [0x%02X/0x%02X] (pos = %d) written\n", ntrk, nsec, pos);
		else
			printf( "Fail to write bloc [0x%02X/0x%02X] (pos = %d)\n", ntrk, nsec, pos);
    }
	return retval;
}

// RCD command

int chngd() {
	int retval = 0;
	if (chdir( param) < 0) {
		retval = 0;
		if (verbose)
			printf( "Cannot change directory to %s\n", param);
	} else {
		getcwd( curdir, 255);
		retval = 1;
		if (verbose)
			printf( "Changing directory to %s\n", curdir);
	}
	return retval;
}

// RMOUNT command

int rmount() {
	char rfilename[256];

	close( imageFile[0].fd);
	if (verbose)
		printf( "closing %s\n", imageFile[0].diskname);

	ready = 1;
	strncpy( rfilename, param, 255);
	strncat( rfilename, ".DSK", 255);	// Rmount don't put the extension
	if (load_dsk( rfilename) < 0) {
		if (verbose)
			printf( "trying with lowercase...\n");
		strncpy( rfilename, param, 255);
		strncat( rfilename, ".dsk", 255);
		if (load_dsk( rfilename) < 0)
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
	
	if (verbose)
		printf( "RDIR( %s) command\n", param);
				
	fputc( CR, serial);
	fputc( LF, serial);

	dirp = opendir( curdir);
	endlist = 1;
	while ((entry = readdir( dirp)) != NULL) {
		if (strcasecmp( (entry->d_name)+strlen(entry->d_name)-3, "DSK") != 0) 
			continue;
		if (strcasestr( entry->d_name, param) != entry->d_name)
			continue;
		if ((reply = fgetc( serial)) != ' ') {
			if (verbose && reply != ESC)
				printf( "Unexpected command (0x%02X) while reading directory\n", reply);
			endlist = 0;
			break;
		}
		if (verbose)
			printf( "---> %s\n", entry->d_name);
		fputs( entry->d_name, serial);
		fputc( CR, serial);
		fputc( LF, serial);
	}
	if (endlist)
		if ((reply = fgetc( serial)) != ' ')
			if (verbose)
				printf( "Unexpected command (0x%02X) while reading directory\n", reply);

	closedir( dirp);
	fputc( ACK, serial);
}

// RLIST command

void lstdir() {
	struct dirent *entry;
	struct stat statbuf;
	DIR *dirp;
	int reply;
	int endlist;

	if (verbose)
		printf( "RLIST command\n");
				
	getparam();
	if ((reply = fgetc( serial)) != 0x20)
		printf( "Bad char 0x%02X received...\n", reply);
	else {
		fputc( CR, serial);
		fputc( LF, serial);
	}

	endlist = 1;
	dirp = opendir( curdir);
	while ((entry = readdir( dirp)) != NULL) {
		if (strcmp(entry->d_name, ".") * strcmp(entry->d_name, "..") == 0)
			continue;
		if (stat( entry->d_name, &statbuf) == -1) {
			if (verbose)
				perror( entry->d_name);
			continue;
		}
		if (S_ISDIR( statbuf.st_mode) == 0) 
			continue;
		if ((reply = fgetc( serial)) != 0x20) {
			if (verbose && reply != ESC)
				printf( "Unexpected command (0x%02X) while reading directory\n", reply);
			endlist = 0;
			break;
		}
		if (verbose)
			printf( "---> %s\n", entry->d_name);
		fputs( entry->d_name, serial);
		fputc( CR, serial);
		fputc( LF, serial);
	}
	if (endlist)
		if ((reply = fgetc( serial)) != ' ')
			if (verbose)
				printf( "Unexpected command (0x%02X) while reading directory\n", reply);
	closedir( dirp);
	fputc( ACK, serial);
}

// Program starts here

int main( int argc, char **argv)
{
	int opt;
	char *name;
//	struct stat dsk_stat;
	int command;
//	int flags;
	struct termios linespec;
	int idlnk;

// Read parameters
	while ((opt = getopt( argc, argv, "d:s:vh")) != -1) {
		switch (opt) {
			case 'h':
				usage( *argv);
				exit( 0);
				break;
			case 'v':
				verbose = 1;
				break;
			case 'd':
				strncpy( line, optarg, 31) ;
				break;
			case 's':
				sscanf( optarg, "%d", &speed);
				break;
			default: /* unknown commands */
				usage( *argv);
				exit( 1);
		}
	}

// Some sanitary checking on options
	if (strlen( line) == 0) {
		fprintf( stderr, "No serial line ?\n");
		usage( *argv);
		exit( 1);
	}

	if (speed == 0) {
		fprintf( stderr, "No baudrate ?\n");
		usage( *argv);
		exit( 1);
	}

	if((serial = fopen( line, "r+")) == NULL )
		perror( line);

	if ((idlnk = fileno( serial)) < 0)
			perror( line);

	if (tcgetattr (idlnk, &linespec) < 0) {
		perror ("ERROR getting current terminal's attributes");
		exit( 1);
	}
	cfmakeraw( &linespec);
	cfsetspeed( &linespec, speed);
		
	if (tcsetattr (idlnk, TCSANOW, &linespec) < 0) {
		perror ("ERROR setting current terminal's attributes");
		exit( 1);
	}

	if (verbose)
		printf( "Link on %s, speed is %d bauds\n", line, speed);

	if (optind < argc) {
		name = argv[ optind++];
		if (optind < argc) {
			fprintf( stderr, "Only one filename is allowed\n");
			usage( *argv);
			exit (1);
		}
	} else {
		fprintf( stderr, "No file name ???\n");
		usage( *argv);
		exit( 1);
	}

	// Load the file


	getcwd( curdir, 255);

	if (load_dsk( name) < 0)
		exit( 1);

	if (readonly) {
		fprintf( stderr, "Flexnet can't start with a read-only file\n");
		exit( 1);
	}

	// Read command from flex side
	
	while (1) {
		command = fgetc( serial);
		*param = 0;
		switch (command) {
			case 0x55:
			case 0xAA:
				fputc( command, serial);
				if (verbose)
					printf( "Initial sync or RESYNC command ($%02x)\n", command);
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
				fputc( ACK, serial);
				if (verbose)
					printf( "Query (change) drive command\n");
				break;
			case '?':	// 
				fputs( curdir, serial);
				fputc( CR, serial);
				fputc( ACK, serial);
				if (verbose)
					printf( "Query current directory (%s) command\n", param);
				break;
			case 'Q':
				fputc( ACK, serial);
				if (verbose)
					printf( "Quick check: is drive ready ? (unix: allways yes)\n");
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
				fputc( NAK, serial);
				if (verbose)
					printf( "%s(%s) command (no action, reply NAK)\n",
						command=='C' ? "RCREATE" : "RDELETE", param);
				break;
			case 'E':	// Flex leave
				fputc (ACK, serial);
				if (verbose)
					printf( "Flexnet exit\n");
				exit( 0);
			case 'P':	// change directory -- param = path
				getparam();
				fputc (chngd() ? ACK : NAK, serial);
				break;
			case 'M':	// mount a new disk image
				getparam();
				if (rmount()) {
					fputc( ACK, serial);
					fputc( readonly ? 'R' : 'W', serial);
				} else
					fputc( NAK, serial);
				break;
			case -1:
				fprintf( stderr, "Serial line disappeared - Panic exit\n");
				exit( 1);
			default:	// WTF ? Something wrong happened... Just ignore...
				if (verbose)
					printf( "Unknown command 0x%02x (%c)\n", command, isprint( command)?command:0);
				break;
		}
	}
}
