#!/bin/sh

dirname=$(pwd | awk 'BEGIN{FS="/"}{print $NF}')
tarball=$dirname-contribution.tar
excludefile=$dirname/.gitignore

cd ..
if [ -e $dirname ]; then
    echo "Contribution directory : $dirname"
else
    echo "Error: Directory $dirname does not exist."
    exit 1
fi

tar -cf $tarball $dirname -X $excludefile
tar -rf $tarball $dirname/.gitrevision
gzip $tarball
echo "Created tarball: ../$tarball. (see the parent directory)"
echo "Please submit this file to pekka.kaitaniemi@gmail.com"
