#!/bin/sh
#
# Copyright (C) 2026 M. Glargaard, aka graybox
#
# This software is provided "as-is", without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would
#    be appreciated but is not required.
#
# 2. Altered source versions must be plainly marked as such, and must not
#    be misrepresented as being the original software.
#
# 3. This notice may not be removed or altered from any source distribution.
#

URI=""
METHOD="GET"
CONTENT_TYPE="application/x-www-form-urlencoded"
CONTENT_LENGTH=0
DEBUG=false
LOGFILE="/tmp/chimera.log"

[ "$DEBUG" = true ] && : > "$LOGFILE"

while IFS= read -r line
do
    line=$(printf "%s" "$line" | tr -d '\r')

    [ "$DEBUG" = true ] && echo "HDR: [$line]" >> "$LOGFILE"

    [ -z "$line" ] && break

    case "$line" in
        GET\ *)
            METHOD="GET"
            ;;
        POST\ *)
            METHOD="POST"
            ;;
        URI:*)
            URI=$(printf "%s" "$line" | sed 's/^URI: //')
            ;;
        Content-type:*)
            CONTENT_TYPE=$(printf "%s" "$line" | sed 's/^Content-type: //')
            ;;
        Content-length:*)
            CONTENT_LENGTH=$(printf "%s" "$line" | sed 's/^Content-length: //')
            ;;
    esac
    
    case "$line" in
		GET\ *|POST\ *)
			REQUEST_LINE="$line"
			;;
		X-hostname:*)
			HOST=$(printf "%s" "$line" | sed 's/^X-hostname: //')
			;;
	esac
done

BODY=""

if [ "$CONTENT_LENGTH" -gt 0 ] 2>/dev/null; then
    BODY=$(dd bs=1 count="$CONTENT_LENGTH" 2>/dev/null)
fi
if [ "$DEBUG" = true ]; then
	echo "REQUEST_LINE=[$REQUEST_LINE]" >> "$LOGFILE"
	echo "HOST=[$HOST]" >> "$LOGFILE"
	echo "METHOD=[$METHOD]" >> "$LOGFILE"
	echo "URI=[$URI]" >> "$LOGFILE"
	echo "CONTENT_TYPE=[$CONTENT_TYPE]" >> "$LOGFILE"
	echo "CONTENT_LENGTH=[$CONTENT_LENGTH]" >> "$LOGFILE"
	echo "BODY=[$BODY]" >> "$LOGFILE"
fi
PATH_AND_QUERY=$(printf "%s\n" "$REQUEST_LINE" | awk '{print $2}')

if [ "$METHOD" = "GET" ] && [ -n "$HOST" ]; then
    URI="https://$HOST$PATH_AND_QUERY"
fi

[ "$DEBUG" = true ] && echo "FINAL_URI=[$URI]" >> "$LOGFILE"

if [ -z "$URI" ]; then
    printf "HTTP/1.0 500 Error\r\n"
    printf "Content-Type: text/plain\r\n"
    printf "\r\n"
    printf "No URI provided\n"
    exit 1
fi

CTYPE=$(${CURL:-curl} -L -s -I \
    -A "Chimera/1.70" \
    "$URI" \
    | tr -d '\r' \
    | awk -F': ' 'BEGIN{IGNORECASE=1} /^Content-Type:/ {print $2}' \
    | tail -1)

if [ -z "$CTYPE" ]; then
    CTYPE="application/octet-stream"
fi

printf "HTTP/1.0 200 OK\r\n"
printf "Content-Type: %s\r\n" "$CTYPE"
printf "\r\n"

if [ "$METHOD" = "POST" ]; then
    ${CURL:-curl} \
        -L \
        -s \
        -A "Chimera/1.70" \
        -H "Content-Type: $CONTENT_TYPE" \
        --data "$BODY" \
        "$URI"
else
    ${CURL:-curl} \
        -L \
        -s \
        -A "Chimera/1.70" \
        "$URI"
fi
