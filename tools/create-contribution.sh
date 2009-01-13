#!/bin/sh

tarball=chep09tmva-contribution.tar.gz
dirname=chep09tmva
cd ..
if [ -e $dirname ]; then
# The default directory name is OK.    
    echo ""
else
    dirname="ah09bProceedings/"
fi

echo "Tarball name : $tarball"
echo "Contribution directory : $dirname"
tar -czf $tarball $dirname
echo "Created tarball: ../$tarball. (see the parent directory)"
echo "Please submit this file to pekka.kaitaniemi@gmail.com"
