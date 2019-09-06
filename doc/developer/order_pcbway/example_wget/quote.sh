#!/bin/sh

# insert API key here:
key=""

server=api-partner.pcbway.com
URL="http://$server/api/Pcb/PcbQuotation"
fn="query.xml"

wget $URL \
	--header "api-key: $key" \
	--header "Content-Type: application/xml" \
	--header "Accept: application/xml" \
	--post-file="$fn"
