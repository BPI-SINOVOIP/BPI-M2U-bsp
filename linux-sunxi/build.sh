#!/bin/bash
set -e

PLATFORM=""
MODULE=""

show_help()
{
	printf "\nbuild.sh - Top level build scritps\n"
	echo "Valid Options:"
	echo "  -h  Show help message"
	echo "  -p <platform> platform, e.g. sun6i, sun6i_fiber or sun6i_dragonboard"
	printf "  -m <module> module\n\n"
}

while getopts hp:m: OPTION
do
	case $OPTION in
	h) show_help
	;;
	p) PLATFORM=$OPTARG
	;;
	m) MODULE=$OPTARG
	;;
	*) show_help
	;;
esac
done

if [ -z "$PLATFORM" ]; then
	show_help
	exit 1
fi

if [ -z "$MODULE" ]; then
	MODULE="all"
fi

if [ -x ./scripts/build_${PLATFORM}.sh ]; then
	./scripts/build_${PLATFORM}.sh $MODULE
else
	printf "\nERROR: Invalid Platform\nonly sun6i sun6i_fiber or sun6i_dragonboard sopport\n"
	show_help
	exit 1
fi



