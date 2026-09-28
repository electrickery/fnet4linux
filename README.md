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

And update the documentation!

fjkraan@electrickery.nl
