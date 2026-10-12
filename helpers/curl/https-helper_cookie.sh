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

#!/bin/sh

URI=""
METHOD="GET"
CONTENT_TYPE="application/x-www-form-urlencoded"
CONTENT_LENGTH=0
#set to true for logging/debug or false to avoid
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
[ ! -d "$HOME/chimera/cookies" ] && mkdir -p "$HOME/chimera/cookies"
COOKIEJAR="$HOME/chimera/cookies/cookies.txt"

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
	echo "COOKIEJAR=[$COOKIEJAR]" >> "$LOGFILE"
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

# Temporary files for headers and the body itself
HEADER_DUMP="/tmp/chimera_headers.$$"
BODY_DUMP="/tmp/chimera_body.$$"

# --- TRICK 1: Block known problem links instantly without network calls ---
# A 1x1 transparent GIF in Base64 format
TRANSPARENT_GIF_B64="R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7"

case "$URI" in
    *flattr*|*badge*|*counter*|*analytics*)
        [ "$DEBUG" = true ] && echo "BLOCKED: Sending dummy image directly for $URI" >> "$LOGFILE"
        printf "HTTP/1.0 200 OK\r\n"
        printf "Content-Type: image/gif\r\n"
        printf "\r\n"
        printf "%s" "$TRANSPARENT_GIF_B64" | base64 -d 2>/dev/null || printf "%s" "$TRANSPARENT_GIF_B64" | openssl enc -d -base64
        exit 0
        ;;
esac

# Run the curl request itself and dump the headers to a file separately
if [ "$METHOD" = "POST" ]; then
    curl.7.47.0 -k -L -s \
        -A "Chimera/1.70" \
        --connect-timeout 3 --max-time 10 \
        -b "$COOKIEJAR" \
        -c "$COOKIEJAR" \
        -H "Content-Type: $CONTENT_TYPE" \
        --data "$BODY" \
        -D "$HEADER_DUMP" \
        -o "$BODY_DUMP" \
        "$URI"
else
    curl.7.47.0 -k -L -s \
        -A "Chimera/1.70" \
        --connect-timeout 3 --max-time 10 \
        -b "$COOKIEJAR" \
        -c "$COOKIEJAR" \
        -D "$HEADER_DUMP" \
        -o "$BODY_DUMP" \
        "$URI"
fi

CURL_EXIT=$?

# Find HTTP status code (eg 200, 404, 500)
HTTP_STATUS=$(tr -d '\r' < "$HEADER_DUMP" 2>/dev/null | awk '/^HTTP\// {print $2}' | tail -1)
[ -z "$HTTP_STATUS" ] && HTTP_STATUS="000"

[ "$DEBUG" = true ] && echo "CURL_EXIT=[$CURL_EXIT], HTTP_STATUS=[$HTTP_STATUS]" >> "$LOGFILE"

# Extract the absolute LAST Content-Type from the header dump (after all redirects)
CTYPE=$(tr -d '\r' < "$HEADER_DUMP" 2>/dev/null \
    | awk -F': ' 'BEGIN{IGNORECASE=1} /^Content-Type:/ {print $2}' \
    | tail -1)

if [ -z "$CTYPE" ]; then
    CTYPE="application/octet-stream"
fi

[ "$DEBUG" = true ] && echo "DETERMINED_CTYPE=[$CTYPE]" >> "$LOGFILE"

# --- TRICK 2: Rescue action if the request failed (e.g. 404 or dead domain) ---
# If the URL looks like an image and the request failed, we send the dummy image
if [ "$CURL_EXIT" -ne 0 ] || [ "$HTTP_STATUS" -ge 400 ]; then
    case "$URI" in
        *.png*|*.gif*|*.jpg*|*.jpeg*|*.ico*)
            [ "$DEBUG" = true ] && echo "RESCUE: Image request failed. Sending 1x1 GIF instead." >> "$LOGFILE"
            printf "HTTP/1.0 200 OK\r\n"
            printf "Content-Type: image/gif\r\n"
            printf "\r\n"
            printf "%s" "$TRANSPARENT_GIF_B64" | base64 -d 2>/dev/null || printf "%s" "$TRANSPARENT_GIF_B64" | openssl enc -d -base64
            rm -f "$HEADER_DUMP" "$BODY_DUMP"
            exit 0
            ;;
    esac
fi

# Send the normal HTTP response if everything went well (or it wasn't an image)
# We pass the correct status code instead of always forcing '200 OK'
if [ "$HTTP_STATUS" = "000" ]; then
    printf "HTTP/1.0 504 Gateway Timeout\r\n"
    printf "Content-Type: text/plain\r\n"
    printf "\r\n"
    printf "The server did not respond in time.\n"
else
    printf "HTTP/1.0 %s\r\n" "$(tr -d '\r' < "$HEADER_DUMP" | awk '/^HTTP\// {print substr($0, index($0,$2))}' | tail -1)"
    printf "Content-Type: %s\r\n" "$CTYPE"
    printf "\r\n"
    cat "$BODY_DUMP"
fi

# Clean up
rm -f "$HEADER_DUMP" "$BODY_DUMP"
