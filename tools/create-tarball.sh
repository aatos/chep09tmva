#!/bin/sh

# Get the directory name
directory=$(pwd | awk 'BEGIN{FS="/"}{print $NF}')

# Clean up
rm -f $directory.tar rm -f $directory.tar.gz .gitrevision
# Get the revision SHA1 ID
git --no-pager log --pretty=oneline | awk '{print $1}' | head -1 > .gitrevision
# Create the archive using Git and add the revision ID file to the end
# of the archive:
git archive --format=tar --prefix=$directory/ HEAD > $directory.tar
cd ..
tar -rf $directory/$directory.tar $directory/.gitrevision
cd $directory
gzip $directory.tar
