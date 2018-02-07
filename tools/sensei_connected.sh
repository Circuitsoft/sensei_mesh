#!/bin/sh

if nrfjprog --memrd 0x1000005c --n 4 | grep -q "0x1000005C: FFFF00E3" ; then
	echo Sensei board is connected.
        exit 0
else
	echo Sensei board is not connected.
        exit 1
fi

