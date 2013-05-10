#!/bin/sh
# Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
# License: GPLv3 or later (see COPYING for details)
#
# Splits out individual Test::Unit test cases to be standalone Ruby
# test files.  This requires Ruby test files to be properly
# formatted/indented.  Outputs a Makefile snippet to stdout of
# all files generated
#
# Usage: test/gen-slow.sh RUBY_TEST_FILES

AWK=${AWK-awk}
set -u
set -e

echo "SLOW_RB_FILES ="

for RB_TEST in "$@"
do
	TESTS="$($AWK '/^  def test_/ { print $2 }' $RB_TEST)"
	case $TESTS in '') continue ;; esac
	CLASS="$($AWK '/^class / && /::TestCase/ { print $2 }' $RB_TEST)"
	case $CLASS
	in '')
		echo "Ruby class name not found in $RB_TEST" >&2
		exit 1
		;;
	esac

	for TEST in $TESTS
	do
		RB_BASENAME="$(basename $RB_TEST)"
		RB_DIRNAME="$(dirname $RB_TEST)"
		RB_PFX=${RB_BASENAME%.rb}
		mkdir -p "$RB_DIRNAME/.$RB_PFX"
		FILE="$RB_DIRNAME/.$RB_PFX/${TEST#test_}.slowrb"
		tmp="$FILE.$$"
		cat > "$tmp" <<EOF
#!/bin/sh -u
# This file is auto-generated with: $0 $RB_TEST
exec "\$RUBY" -I"\$top_srcdir" -w "\$top_srcdir"/$RB_TEST -n $TEST
EOF
		chmod +x "$tmp"
		mv "$tmp" "$FILE"
		echo "SLOW_RB_FILES += $FILE"
	done
done
