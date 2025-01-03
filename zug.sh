#! /bin/bash

# Wrapper zum Aufruf von Mercator

TMPDIR=/tmp
TMPEXT=.tmp
MPATH=/home/donchaos/eressea/mercator
MBIN=mercator
MOPT="-st traditional -sc 2"
NR=cat
PICONV=/usr/bin/piconv

usage() {
    echo "$0 workdir zugnr karte html  -- mache Eressea-HTML"
}


if [ $# != 4 ] ; then
    usage
    exit 1
fi

WORKDIR=$1
TURN=$2
MAP=$3
HTML=$4
OLDTURN=$TURN
let OLDTURN=OLDTURN-1

if ! test -f $WORKDIR/$TURN-$NR.cr ; then
    echo "CR $WORKDIR/$TURN-$NR.cr not found"
    exit 1
fi

if ! test -f $WORKDIR/$OLDTURN-$NR.cr ; then
    echo "CR $WORKDIR/$OLDTURN-$NR.cr not found"
    exit 1
fi

if ! test -f $WORKDIR/$MAP ; then
    echo "Map file $WORKDIR/$MAP not found"
    exit 1
fi

if ! test -d $WORKDIR/$HTML ; then
    echo "HTML directory $WORKDIR/$HTML not found"
    exit 1
fi

$PICONV -f utf8 -t iso8859-1 < $WORKDIR/$TURN-$NR.cr > $TMPDIR/$TURN-$NR.cr
$PICONV -f utf8 -t iso8859-1 < $WORKDIR/$OLDTURN-$NR.cr > $TMPDIR/$OLDTURN-$NR.cr

# generate map
echo "generiere Karte ..."
$MPATH/$MBIN read-cr $WORKDIR/$MAP merge-cr $TMPDIR/$TURN-$NR.cr write-map $WORKDIR/$MAP

# generate HTML
echo "generiere HTML ..."
$MPATH/$MBIN $MOPT read-cr $WORKDIR/$MAP merge-cr $TMPDIR/$OLDTURN-$NR.cr merge-cr $TMPDIR/$TURN-$NR.cr write-html $WORKDIR/$HTML

