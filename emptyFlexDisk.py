#!/bin/python
#

# Flex Disk Image Creator - Creates an image for the NetPC/FlexNet server.
#    The current version only creates one size: double sided, double 
#    density, 80 tracks.
#
# Usage: python3 [<emptyFlexDisk.py> [<fileImageName.EXT> [<volumeName>] [<volumeNumber]]]
#
#  <fileImageName.EXT> is a host file system compatible name. EXT is usally DSK or IMA.
#  <volumeName> can be eight characters
#  <volumeNumber> the volume number is two characters 
#

version = "0.2"

import sys
import datetime

if len(sys.argv) > 1:
    diskFile = sys.argv[1]
else:
    diskFile = "Empty-DSDD80-5.DSK"
    
if len(sys.argv) > 2:
    sirVolName = bytes((sys.argv[2]+"           ")[0:8], "utf-8")
else:
    sirVolName = b'EMPTY\00\00\00\00\00\00'
    
if len(sys.argv) > 3:
    sirVolNum = bytes((sys.argv[3])[0:3], "utf-8")
else:
    sirVolNum = b'\00\01'
    
today = datetime.datetime.now()

todayDay = int(today.strftime("%d"))
todayMonth = int(today.strftime("%m"))
todayYear = int(today.strftime("%y"))

sectorSize = 256
track0Sectors = 36
track1UpTracks = 79
track1UpSectors = 36
totalSectors = track1UpTracks * track1UpSectors
totalSecMSB = int(totalSectors / 256)
totalSecLSB = int((totalSectors - totalSecMSB * 256) % 256)

#sirVolName = b'EMPTY      '     # $10 - $1C
#sirVolNum = b'\x00\x01'         # $1D - $1E
sirFirstFreeTrk = b'\x01'       # $1F
sirFirstFreeSec = b'\x01'       # $20
sirFreeSecCntM = bytes([totalSecMSB])  # b'\x0b'     # $21
sirFreeSecCntL = bytes([totalSecLSB])  # b'\xc1'     # $22
sirMonth = bytes([todayMonth])  # b'\x0a'              # $23
sirDay   = bytes([todayDay])      # b'\x09'                # $24
sirYear  = bytes([todayYear])    # b'\x1a'               # $25
sirEndTrk = bytes([track1UpTracks])  # b'\x4f'             # $26
sirEndSec = bytes([track1UpSectors]) # b'\x24'             # $27
sirFiller = b'\x00\x00\x00\x00\x00\x00\x00\x00'


oFile = open(diskFile, "wb")

# track 0
#  boot sectors
for i in range (sectorSize * 2):
    oFile.write(b'\00')
    
# sir sector1
for i in range (16):
    oFile.write(b'\00')
    
oFile.write(sirVolName)
oFile.write(sirVolNum)
oFile.write(sirFirstFreeTrk)
oFile.write(sirFirstFreeSec)
oFile.write(sirEndTrk)
oFile.write(sirEndSec)
oFile.write(sirFreeSecCntM)
oFile.write(sirFreeSecCntL)
oFile.write(sirMonth)
oFile.write(sirDay)
oFile.write(sirYear)
oFile.write(sirEndTrk)
oFile.write(sirEndSec)
oFile.write(sirFiller)
for i in range (208):
    oFile.write(b'\00')

# sir sector2
oFile.write(b'\00')
oFile.write(b'\05')
for i in range (2, 256):
    oFile.write(b'\00')
    
# directory sectors
for i in range (6, track0Sectors+1):
    oFile.write(b'\00')
    oFile.write(bytes([i]))
    for j in range (2, 256):
        oFile.write(b'\00')

# track 1 and up
for t in range(1, track1UpTracks+1):
    for s in range(1, track1UpSectors+1):
        oFile.write(bytes([t]))
        oFile.write(bytes([s]))
        for j in range(2, 256):
            oFile.write(b'\00')

# last 
for i in range (256):
    oFile.write(b'\00')

oFile.close()
