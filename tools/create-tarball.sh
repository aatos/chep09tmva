#!/bin/sh

function dirty_tree
{
    echo "You have uncommitted changes in your tree."
    echo "Please use git add and git commit to commit them before trying to create a tarball."
    exit -1
}

# Do not create a tarball if the working directory contains
# uncommitted changes (i.e. it is a "dirty tree"):
git update-index --refresh -q
test -z "$(git diff-index --name-only HEAD --)" ||
dirty_tree

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
