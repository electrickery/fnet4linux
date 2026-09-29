# fnet4linux

Based on Michael Wurtz' Fnet4linux code.

# What is it?

Fnet4linux is an implementation of the NetPC/FlexNet protocol designed by Bjarne Bäckström 
and Ron Anderson. Implementations of this protocol allow a floppy disk interface to be replaced
by a serial port. The contents of the floppy is a file on a PC. The protocol is noet very 
demanding and can be implemented on an original Arduino and SD-card, while still support four 
disk drives.

## Summary of changes to make the code multi-disk:

* By moving some image related variables to a struct and add four of those to an
array, the original code becomes multi-disk. 
* The single argument for the disk-image
is replaced by up to four options '-0, -1, -2 and -3) each with an argument.
* Added a makefile.

## Info on the hardware used

The page describing the hardware and modifications to the monitor and flex drivers is here:
https://electrickery.nl/comp/more6809/keesFlex/.

## ToDo

What remains is checking the other netPC/FlexNet commands for proper working with
more images and cleanup.

And update the documentation! In the meantime, here an example:

    flexnet -d /dev/ttyUSB2 -s 9600 -v -0 disks/FLEXCMI.DSDD80-5f2.IMA -1 'disks/EDITSRC.DSK' -2 'disks/GAMES.DSK'

fjkraan@electrickery.nl
