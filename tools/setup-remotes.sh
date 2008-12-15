#!/bin/sh

# Configuration
simulatemode=1 # Don't do anything, only display commands that would be executed
verbosemode=0  # Print extra messages on screen
helpmode=1     # Show help and exit

# Run command or display it on the screen, depending on the
# configuration options given on the command line.
function run_command
{
    command=$1
    if test "$simulatemode" -eq 1; then
	echo -n "simulate: would run command: "
	echo $command
    else
	$command
    fi
}

# Add a remote URL to the repository
function add_remote ()
{
    $name = $1
    $url = $2
    git remote add $name $url
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
    echo $arg
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

