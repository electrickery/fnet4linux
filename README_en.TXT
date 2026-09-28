Archive's Content:

flexnet.c : Linux flexnet server ("make flexnet" to compile)
fnlinux.pdf : Update to original FNMANUAL.TXT
fnlinux.dsk : Flex Disk Image (Single Side, Single Density, 80 tracks)
              binaries and source of all Flex utilities + manuals

Assembled with a09. Warinin : use a recent version since the original one fails.
Use at least the one labelled "Copyright (C) 2024 J. Bertrand"

With Flex, run "flexnet 3" to mount disk #3 from Linux
With Linux, run something like "flexnet -v -d /dev/ttyUSB0 -b 19200 fnlinux.dsk"
Replace parameters with your own (tty, speed and disk image)

Content of fnlinux.dsk :

File list (24 used + 0 deleted / 60 entries):
 id       Filename  start    end    size      date    flags
  1    FLEXNET.asm [01/01 - 0a/02]    92   2025-04-30
  2    FLEXNET.CMD [0a/03 - 0a/07]     5   2025-04-30
  3    FNLINUX.TXT [0a/08 - 0c/02]    15   2025-04-30
  4   FNMANUAL.TXT [0c/03 - 17/06]   114   2025-04-30
  5        RCD.asm [17/07 - 19/08]    22   2025-04-21
  6        RCD.CMD [19/09 - 19/0a]     2   2002-09-19
  7    RCREATE.CMD [1a/01 - 1a/03]     3   2002-09-19
  8    RCREATE.TXT [1a/04 - 1c/03]    20   2025-04-13
  9    RDELETE.CMD [1c/04 - 1c/06]     3   2002-11-26
 10    RDELETE.TXT [1c/07 - 1e/03]    17   2025-04-13
 11       RDIR.asm [1e/04 - 20/06]    23   2025-04-27
 12       RDIR.CMD [20/07 - 20/08]     2   2025-04-27
 13     RDRIVE.asm [20/09 - 23/07]    29   2025-04-19
 14     RDRIVE.CMD [23/08 - 23/09]     2   2002-11-24
 15     RESYNC.asm [23/0a - 25/06]    17   2025-04-18
 16     RESYNC.CMD [25/07 - 25/08]     2   2002-09-19
 17      REXIT.asm [25/09 - 27/0a]    22   2025-04-18
 18      REXIT.CMD [28/01 - 28/02]     2   2002-11-29
 19      RLIST.asm [28/03 - 2a/04]    22   2025-04-27
 20      RLIST.CMD [2a/05 - 2a/06]     2   2025-04-27
 21     RMOUNT.asm [2a/07 - 2d/04]    28   2025-04-20
 22     RMOUNT.CMD [2d/05 - 2d/06]     2   2002-11-14
 23     RWHICH.asm [2d/07 - 2f/03]    17   2025-04-19
 24     RWHICH.CMD [2f/04 - 2f/05]     2   2002-11-23

