#!/bin/sh

URI=""
METHOD="GET"
CONTENT_TYPE="application/x-www-form-urlencoded"
CONTENT_LENGTH=0

LOGFILE="/tmp/chimera.log"

: > "$LOGFILE"

while IFS= read -r line
do
    line=$(printf "%s" "$line" | tr -d '\r')

    echo "HDR: [$line]" >> "$LOGFILE"

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

echo "REQUEST_LINE=[$REQUEST_LINE]" >> "$LOGFILE"
echo "HOST=[$HOST]" >> "$LOGFILE"
echo "METHOD=[$METHOD]" >> "$LOGFILE"
echo "URI=[$URI]" >> "$LOGFILE"
echo "CONTENT_TYPE=[$CONTENT_TYPE]" >> "$LOGFILE"
echo "CONTENT_LENGTH=[$CONTENT_LENGTH]" >> "$LOGFILE"
echo "BODY=[$BODY]" >> "$LOGFILE"
echo "COOKIEJAR=[$COOKIEJAR]" >> "$LOGFILE"

PATH_AND_QUERY=$(printf "%s\n" "$REQUEST_LINE" | awk '{print $2}')

if [ "$METHOD" = "GET" ] && [ -n "$HOST" ]; then
    URI="https://$HOST$PATH_AND_QUERY"
fi

echo "FINAL_URI=[$URI]" >> "$LOGFILE"

if [ -z "$URI" ]; then
    printf "HTTP/1.0 500 Error\r\n"
    printf "Content-Type: text/plain\r\n"
    printf "\r\n"
    printf "No URI provided\n"
    exit 1
fi

# Midlertidige filer til headers og selve bodyen
HEADER_DUMP="/tmp/chimera_headers.$$"
BODY_DUMP="/tmp/chimera_body.$$"

# --- TRICK 1: Bloker kendte problem-links omgående uden netværkskald ---
# En 1x1 gennemsigtig GIF i Base64-format
TRANSPARENT_GIF_B64="R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7"

case "$URI" in
    *flattr*|*badge*|*counter*|*analytics*)
        echo "BLOKERET: Sender dummy-billede direkte for $URI" >> "$LOGFILE"
        printf "HTTP/1.0 200 OK\r\n"
        printf "Content-Type: image/gif\r\n"
        printf "\r\n"
        printf "%s" "$TRANSPARENT_GIF_B64" | base64 -d 2>/dev/null || printf "%s" "$TRANSPARENT_GIF_B64" | openssl enc -d -base64
        exit 0
        ;;
esac

# Kør selve curl-anmodningen og dump headers til en fil separat
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

# Find HTTP statuskode (f.eks. 200, 404, 500)
HTTP_STATUS=$(tr -d '\r' < "$HEADER_DUMP" 2>/dev/null | awk '/^HTTP\// {print $2}' | tail -1)
[ -z "$HTTP_STATUS" ] && HTTP_STATUS="000"

echo "CURL_EXIT=[$CURL_EXIT], HTTP_STATUS=[$HTTP_STATUS]" >> "$LOGFILE"

# Træk den absolut SIDSTE Content-Type ud af header-dumpet (efter alle redirects)
CTYPE=$(tr -d '\r' < "$HEADER_DUMP" 2>/dev/null \
    | awk -F': ' 'BEGIN{IGNORECASE=1} /^Content-Type:/ {print $2}' \
    | tail -1)

if [ -z "$CTYPE" ]; then
    CTYPE="application/octet-stream"
fi

echo "DETERMINED_CTYPE=[$CTYPE]" >> "$LOGFILE"

# --- TRICK 2: Redningsaktion hvis anmodningen fejlede (f.eks. 404 eller dødt domæne) ---
# Hvis URL'en ligner et billede, og anmodningen fejlede, sender vi dummy-billedet
if [ "$CURL_EXIT" -ne 0 ] || [ "$HTTP_STATUS" -ge 400 ]; then
    case "$URI" in
        *.png*|*.gif*|*.jpg*|*.jpeg*|*.ico*)
            echo "REDNING: Forespørgsel på billede fejlede. Sender 1x1 GIF i stedet." >> "$LOGFILE"
            printf "HTTP/1.0 200 OK\r\n"
            printf "Content-Type: image/gif\r\n"
            printf "\r\n"
            printf "%s" "$TRANSPARENT_GIF_B64" | base64 -d 2>/dev/null || printf "%s" "$TRANSPARENT_GIF_B64" | openssl enc -d -base64
            rm -f "$HEADER_DUMP" "$BODY_DUMP"
            exit 0
            ;;
    esac
fi

# Send den normale HTTP-respons hvis alt gik godt (eller det ikke var et billede)
# Vi videresender den rigtige statuskode i stedet for altid at tvinge '200 OK'
if [ "$HTTP_STATUS" = "000" ]; then
    printf "HTTP/1.0 504 Gateway Timeout\r\n"
    printf "Content-Type: text/plain\r\n"
    printf "\r\n"
    printf "Serveren svarede ikke i tide.\n"
else
    printf "HTTP/1.0 %s\r\n" "$(tr -d '\r' < "$HEADER_DUMP" | awk '/^HTTP\// {print substr($0, index($0,$2))}' | tail -1)"
    printf "Content-Type: %s\r\n" "$CTYPE"
    printf "\r\n"
    cat "$BODY_DUMP"
fi

# Ryd op efter os selv
rm -f "$HEADER_DUMP" "$BODY_DUMP"
