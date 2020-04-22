#!/bin/sh

if test $# -lt 3
then
	echo "Usage: ./sign.sh privkey cert file1 [file2 .. fileN]"
fi

key="$1"
cert="$2"
shift 1

for fn in "$@"
do
	openssl dgst -sha256 -sign "$key" -out "$fn.sig" "$fn"
	cp "$cert" "$fn.crt"
done

