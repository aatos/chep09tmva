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

# Set newline as our desired field separator. Without this the config
# file will not be read properly!
IFS="
"

# Configuration
simulatemode=0 # Don't do anything, only display commands that would be executed
verbosemode=0  # Print extra messages on screen
helpmode=0     # Show help and exit

# Run command or display it on the screen, depending on the
# configuration options given on the command line.
function run_command
{
    command=$1
    if test "$simulatemode" -eq 1; then
	echo -n "simulate: would run command: "
	echo $command
    else
	if test "$verbosemode" -eq 1; then
	    echo $command
	fi
	eval $command
    fi
}

# Add a remote URL to the repository
function add_remote ()
{
    name=$1
    url=$2
    command="git remote add $name $url"
    run_command $command
}

function add_repositories ()
{
    configfile=$1
    if test "$verbosemode" -eq 1; then
	echo -n "Using config file: "
	echo $configfile
    fi
    for line in $(cat $configfile); do
	name=$(echo $line | awk '{print $1}')
	url=$(echo $line | awk '{print $2}')
	if test "$verbosemode" -eq 1; then
	    echo -n "Remote name: "
	    echo $name
	    echo -n "URL: "
	    echo $url
	fi
	add_remote $name $url
    done;
}

# Show help
function usage ()
{
    echo "Usage:"
    echo "setup-remotes.sh [--verbose|--help|--simulate]"
    echo ""
    echo " --simulate       Don't do anything. Only print commands on the screen"
    echo " --verbose        Print detailed messages"
    echo " --help           Print this message"
}

# End program cleanly (error code 0) or with an error message (error
# code -1).
function die ()
{
    errcode=$1     # Error code: 0 = normal exit, -1 error
    message=$2
    if test "$errcode" -ne 0; then
	echo -n $0
	echo -n " : "
	echo $message
    fi
    exit $errcode
}

# Main program:

# Parse command line parameters:
for arg in $*; do
    if test "$arg" = "--verbose"; then
	verbosemode=1
    fi
    if test "$arg" = "--simulate"; then
	simulatemode=1
    fi
    if test "$arg" = "--help"; then
	helpmode=1
    fi
done

if test "$helpmode" -eq 1; then
    usage
    die 0 ""
fi

# Configure Git to not verify SSL certificates. This is needed to
# access repositories hosted on http://cmsdoc.cern.ch.
git config http.sslVerify false

if [ -e "tools/repositories.conf" ]; then # Check if we are in the main directory
    add_repositories "tools/repositories.conf"
else
    if [ -e "./repositories.conf" ]; then # Check if we are in tools/
	add_repositories "./repositories.conf"
    else
	if [ -e "../tools/repositories.conf" ]; then # Check if we are in a subdirectory
	    add_repositories "../tools/repositories.conf"
	else
	    die -1 "Repository config file not found."
	fi
    fi
fi

