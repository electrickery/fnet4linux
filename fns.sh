#!/bin/sh
#
# Usage: sh fns.sh <Image0> [<Image1>] [<Image2>] [<Image3>]

IMAGE0=$1
IMAGE1=$2
IMAGE2=$3
IMAGE3=$4

if [ "$IMAGE0" != "" ]
then
    OPT0=`echo -0 $IMAGE0`
fi
if [ "$IMAGE1" != "" ]
then
    OPT1=`echo -1 $IMAGE1`
fi
if [ "$IMAGE2" != "" ]
then
    OPT2=`echo -2 $IMAGE2`
fi
if [ "$IMAGE3" != "" ]
then
    OPT3=`echo -3 $IMAGE3`
fi

EXE=./flexnet 

PORT=/dev/ttyUSB2
SPEED=9600

echo $EXE -d $PORT -s $SPEED $OPT0 $OPT1 $OPT2 $OPT3 -v
$EXE -d $PORT -s $SPEED $OPT0 $OPT1 $OPT2 $OPT3 -v
