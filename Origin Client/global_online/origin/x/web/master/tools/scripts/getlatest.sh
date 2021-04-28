#!/bin/bash
# Written by: D'arcy Gog (dgog@ea.com)
# Created: March 11. 2015
# this script will return the latest build version given a number.
# You specify the branch name and app. It will return a version number in a file called "latest" in the current dir.

function error {
	tput setaf 1; echo $1
	tput sgr0
}

function usage {
	echo "Usage: getversion <branch> <app>"
	echo "Branch = source branch."
	echo "App = spa or mapper"
	exit 1
}

if [ $# -ne 2 ]; then
	error "Error: Incorrect number of arguments."
	echo
	usage
fi
 
# check to see if these is a branch name passed in.
if [ "$1" == "" ]; then
	echo "Error: You must enter in a branch name as an argument."
	echo "eg. \"getlatest DL\""
	usage
fi

if [ "$2" == "spa" ]; then
	APP=originwebapp
elif [ "$2" == "mapper" ]; then
	APP=origincms
else
	echo "Error: Second argument is not \"spa\" or \"mapper\""
	usage
fi

BRANCH="$1"
OUTPUT_FILE=$1-latest
MAVEN_ROOT=http://maven.dm.origin.com/maven/com/ea/origin
MAVEN_LATEST_FILE=$MAVEN_ROOT/x/sandboxes/$BRANCH/$APP/latest.txt

# delete the latest version file
rm -f $OUTPUT_FILE
# delete the wget output file 
rm -f output

echo Branch is $BRANCH
echo Getting latest version from $MAVEN_LATEST_FILE

# just check to see if the files exists but don't download it
wget --spider -o output $MAVEN_LATEST_FILE
# if 404 Not Found is in the output, the latest file does not exist
grep -q "200 OK" ./output
if [ $? -eq 0 ]; then
	# the -N tells wget to only get the file if server version is newer
	wget -q -N -O $OUTPUT_FILE $MAVEN_LATEST_FILE
	echo Latest version in $BRANCH: `cat $OUTPUT_FILE`
else
	echo latest file not found. Cannot determine latest version
fi
