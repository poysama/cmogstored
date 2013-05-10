#!/bin/sh
>> test/slow.mk
if gnulib-tool --update && autoreconf -i
then
	exit 0
fi
cat HACKING
exit 1
