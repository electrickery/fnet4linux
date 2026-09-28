# fnet4linux

Based on Michael Wurtz' code.

## Summary of changes to make the code multi-disk:

By moving some image related variables to a struct and add four of those to an
array, the original code becomes multi-disk. The single argument for the disk-image
is replaced by up to four options '-0, -1, -2 and -3) each with an argument.
And added a makefile.

## ToDo

What remains is checking the other netPC/FlexNet commands for proper working with
more images and cleanup.

And update the documentation! In the meantime, here an example:

    flexnet -d /dev/ttyUSB2 -s 9600 -v -0 disks/FLEXCMI.DSDD80-5f2.IMA -1 'disks/EDITSRC.DSK' -2 'disks/GAMES.DSK'

fjkraan@electrickery.nl
