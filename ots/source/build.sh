#!/bin/bash
#Huczu
#clean
rm -rf *.o *.gch

# Get number cores
CORES=`grep processor /proc/cpuinfo | wc -l`
# Set make processes - 1 + number of cores
MAKEOPT=$(($CORES + 1))
echo ""
echo "Start building on $CORES cores, using $MAKEOPT processes"
echo ""
make -j $MAKEOPT
