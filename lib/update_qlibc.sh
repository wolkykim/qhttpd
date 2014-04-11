#!/bin/sh

OLDQLIBC="./qlibc"
NEWQLIBC="./qlibc_new"
PATCH="qlibc.patch"

## Clean old
if [ -f $OLDQLIBC/Makefile ]; then
    (cd $OLDQLIBC/src; make cleandoc)
    (cd $OLDQLIBC; make distclean)
fi

# Get new
if [ -d $NEWQLIBC ]; then
    rm -rf $NEWQLIBC
fi

echo "Retrieving files from SVN repo..."
svn export https://svn.qdecoder.org/qlibc/trunk $NEWQLIBC > /dev/null

# Diff
DIFF=`diff -r $OLDQLIBC $NEWQLIBC`
if [ ! "$DIFF" = "" ]; then
    #diff -rupN $OLDQLIBC $NEWQLIBC > $PATCH
    diff -rup $OLDQLIBC $NEWQLIBC > $PATCH
    echo "--[DIFF]---------------------------------------------------------"
    cat $PATCH
    echo "-----------------------------------------------------------------"
    echo "Patching files..."
    patch -p0 --posix < $PATCH

    ## Check again
    VERIFY=`diff -r $OLDQLIBC $NEWQLIBC`
    if [ ! "$VERIFY" = "" ]; then
        echo "--[VERIFY]-------------------------------------------------------"
        echo "$VERIFY"
        echo "-----------------------------------------------------------------"
        echo "Update completed but there are something need to be handled."
    else
        echo "Update completed."
    fi
else
    echo "Completed - No changes."
fi

# Clean up
#if [ -d $NEWQLIBC ]; then
#    rm -r $NEWQLIBC
#fi

#if [ -f $PATCH ]; then
#    rm $PATCH
#fi
