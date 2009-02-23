#!/bin/sh

# Copyright (c) 2008, Pekka kaitaniemi
#
# Permission to use, copy, modify, and/or distribute this software for
# any purpose with or without fee is hereby granted, provided that the
# above copyright notice and this permission notice appear in all
# copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
# WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
# AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
# DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
# OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
# TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

# Options
vismode=1
fetchmode=1
fetchorigin=0

# Get registered remote names and filter out origin (i.e. the user's
# own public repository.
function get_remotes ()
{
    if test "$fetchorigin" -eq 1; then
	echo $(git remote | xargs)
    else
	echo $(git remote | sed '/origin/ d' | sed '/private/ d' | xargs)
    fi
}

function usage ()
{
    echo "fetch-all.sh"
    echo "Options:"
    echo " --no-vis        Do not show gitk history visualization"
    echo " --no-fetch      Do not fetch changes"
    echo " --fetch-origin  Fetch also from origin"
    exit 0
}

for option in $*; do
    if test "$option" = "--no-vis"; then
	vismode=0
    fi
    if test "$option" = "--no-fetch"; then
	fetchmode=0
    fi
    if test "$option" = "--fetch-origin"; then
	fetchorigin=1
    fi
    if test "$option" = "--help"; then
	usage
    fi
done

if test "$fetchmode" -eq 1; then
    res=$(get_remotes)
    branches="master"
    for repository in $(get_remotes); do
	echo -n "Fetching from: "
	echo $repository
	eval git fetch $repository
    done
fi

# Spawn gitk viewer to visualize the state of different fetched
# branches:
if test "$vismode" -eq 1; then
    for repository in $(get_remotes); do
	branches="$branches $repository/master"
    done

    gitk --date-order $branches &
fi
