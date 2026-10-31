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

#
# https-helper.sh - HTTPS for Chimera through curl
#
# Chimera has no TLS code. The "protocol" file maps https to this script.
# Chimera writes an HTTP/1.0 style request on stdin; the script fetches
# the page with curl and writes a plain HTTP/1.0 response on stdout.
#
# Settings, each can also be given as an environment variable:
#
#   CURL      curl program to run. Default: chimera-curl if installed
#             (the static build from helpers/curl), otherwise curl.
#   COOKIES   true: keep cookies in $HOME/chimera/cookies/cookies.txt
#             false: no cookies are stored or sent
#             Images get the cookies only when they come from the same
#             site as the page (Chimera says which), and never store
#             any: no third-party cookies.
#   CA_BUNDLE file with trusted root certificates, passed to curl with
#             --cacert. Default: $HOME/chimera/cacert.pem if it exists,
#             otherwise curl's own default store. Useful when the system
#             store is missing or old; "make -C helpers/curl cacert"
#             downloads a current one.
#   INSECURE  true: do not check server certificates (curl -k). Last
#             resort when no usable CA_BUNDLE is available.
#   DEBUG     true: log each request, including form data, to $LOGFILE
#             (default $HOME/chimera/helper.log). Leave false in normal use.
#
# Redirects are followed only to https addresses: when an https page
# redirects to plain http, a short page with the link is shown instead,
# so that nothing is fetched unencrypted without the user seeing it.
#

if [ -z "$CURL" ]; then
    CURL=$(command -v chimera-curl 2>/dev/null) || CURL=curl
fi
COOKIES=${COOKIES:-true}
if [ -z "$CA_BUNDLE" ] && [ -f "$HOME/chimera/cacert.pem" ]; then
    CA_BUNDLE="$HOME/chimera/cacert.pem"
fi
INSECURE=${INSECURE:-false}
DEBUG=${DEBUG:-false}
LOGFILE=${LOGFILE:-$HOME/chimera/helper.log}

URI=""
HOST=""
INLINE=false
PAGE_HOST=""
REQUEST_LINE=""
METHOD="GET"
CONTENT_TYPE="application/x-www-form-urlencoded"
CONTENT_LENGTH=0

log() {
    [ "$DEBUG" = true ] && echo "$*" >> "$LOGFILE"
}

[ "$DEBUG" = true ] && : > "$LOGFILE"

# --- Read the request headers from Chimera ---
while IFS= read -r line
do
    line=$(printf "%s" "$line" | tr -d '\r')
    log "HDR: [$line]"

    [ -z "$line" ] && break

    case "$line" in
        GET\ *)
            METHOD="GET"
            REQUEST_LINE="$line"
            ;;
        POST\ *)
            METHOD="POST"
            REQUEST_LINE="$line"
            ;;
        URI:*)
            URI=$(printf "%s" "$line" | sed 's/^URI: //')
            ;;
        X-hostname:*)
            HOST=$(printf "%s" "$line" | sed 's/^X-hostname: //')
            ;;
        X-chimera-inline:*)
            # an image, possibly loaded alongside others
            INLINE=true
            ;;
        X-chimera-page:*)
            # the host of the page the image is on
            PAGE_HOST=$(printf "%s" "$line" | sed 's/^X-chimera-page: //')
            ;;
        Content-type:*)
            CONTENT_TYPE=$(printf "%s" "$line" | sed 's/^Content-type: //')
            ;;
        Content-length:*)
            CONTENT_LENGTH=$(printf "%s" "$line" | sed 's/^Content-length: //')
            ;;
    esac
done

BODY=""
if [ "$CONTENT_LENGTH" -gt 0 ] 2>/dev/null; then
    BODY=$(dd bs=1 count="$CONTENT_LENGTH" 2>/dev/null)
fi

PATH_AND_QUERY=$(printf "%s\n" "$REQUEST_LINE" | awk '{print $2}')
if [ "$METHOD" = "GET" ] && [ -n "$HOST" ]; then
    URI="https://$HOST$PATH_AND_QUERY"
fi

log "METHOD=[$METHOD] URI=[$URI] CURL=[$CURL] CA_BUNDLE=[$CA_BUNDLE] INSECURE=[$INSECURE]"
log "CONTENT_TYPE=[$CONTENT_TYPE] CONTENT_LENGTH=[$CONTENT_LENGTH]"
log "BODY=[$BODY]"

if [ -z "$URI" ]; then
    printf "HTTP/1.0 500 Error\r\n"
    printf "Content-Type: text/plain\r\n"
    printf "\r\n"
    printf "No URI provided\n"
    exit 1
fi

# --- Block known tracking links without a network call ---
# A 1x1 transparent GIF
TRANSPARENT_GIF_B64="R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7"

send_transparent_gif() {
    printf "HTTP/1.0 200 OK\r\n"
    printf "Content-Type: image/gif\r\n"
    printf "\r\n"
    printf "%s" "$TRANSPARENT_GIF_B64" | base64 -d 2>/dev/null \
        || printf "%s" "$TRANSPARENT_GIF_B64" | openssl enc -d -base64
}

case "$URI" in
    *flattr*|*badge*|*counter*|*analytics*)
        log "BLOCKED: $URI"
        send_transparent_gif
        exit 0
        ;;
esac

# --- Fetch with curl ---
# Temporary files, removed again on exit
HEADER_DUMP=$(mktemp "${TMPDIR:-/tmp}/chimera_headers.XXXXXX") || exit 1
BODY_DUMP=$(mktemp "${TMPDIR:-/tmp}/chimera_body.XXXXXX") || { rm -f "$HEADER_DUMP"; exit 1; }
trap 'rm -f "$HEADER_DUMP" "$BODY_DUMP"' EXIT

# -L with --proto-redir: redirects to https only (see the top)
set -- -L --proto-redir =https --max-redirs 10 \
       -s -A "Chimera/1.70" --connect-timeout 3 --max-time 10 \
       -D "$HEADER_DUMP" -o "$BODY_DUMP"

# The site of a host name, roughly its registrable domain: the last two
# labels (dr.dk, cnn.com), or three when the second last is one of the
# usual second-level names under a country code (bbc.co.uk). An IP
# address is its own site.
site() {
    printf "%s\n" "$1" | tr 'A-Z' 'a-z' | sed 's/:[0-9]*$//' | awk -F. '
        /^[0-9.]+$/ { print; next }
        NF >= 3 && length($NF) == 2 &&
        $(NF-1) ~ /^(co|com|net|org|gov|edu|ac|or|ne|go|gob|mil|nom|ltd|plc|sch|gv|lg)$/ {
            print $(NF-2) "." $(NF-1) "." $NF; next }
        NF >= 2 { print $(NF-1) "." $NF; next }
        { print }'
}

if [ "$COOKIES" = true ]; then
    COOKIEJAR="$HOME/chimera/cookies/cookies.txt"
    mkdir -p "$HOME/chimera/cookies"
    if [ "$INLINE" = true ]; then
        # Images never write the cookie file: Chimera loads several at
        # once, and they would overwrite each other's changes; and an
        # image from another site must not set cookies (tracking). They
        # send cookies only to the page's own site.
        if [ -n "$PAGE_HOST" ] && [ "$(site "$HOST")" = "$(site "$PAGE_HOST")" ]; then
            set -- "$@" -b "$COOKIEJAR"
            log "COOKIES: image from the page's site $HOST, sent"
        else
            log "COOKIES: image from $HOST on a page from [$PAGE_HOST], not sent"
        fi
    else
        set -- "$@" -b "$COOKIEJAR" -c "$COOKIEJAR"
    fi
fi

if [ "$INSECURE" = true ]; then
    set -- "$@" -k
elif [ -n "$CA_BUNDLE" ]; then
    set -- "$@" --cacert "$CA_BUNDLE"
fi

if [ "$METHOD" = "POST" ]; then
    set -- "$@" -H "Content-Type: $CONTENT_TYPE" --data "$BODY"
fi

"$CURL" "$@" "$URI"
CURL_EXIT=$?

# HTTP status code of the last response (after redirects)
HTTP_STATUS=$(tr -d '\r' < "$HEADER_DUMP" 2>/dev/null | awk '/^HTTP\// {print $2}' | tail -1)
[ -z "$HTTP_STATUS" ] && HTTP_STATUS="000"

# Content-Type of the last response (after redirects)
CTYPE=$(tr -d '\r' < "$HEADER_DUMP" 2>/dev/null \
    | awk -F': ' 'tolower($1) == "content-type" {print $2}' \
    | tail -1)
[ -z "$CTYPE" ] && CTYPE="application/octet-stream"

log "CURL_EXIT=[$CURL_EXIT] HTTP_STATUS=[$HTTP_STATUS] CTYPE=[$CTYPE]"

# --- A redirect from https to plain http was not followed ---
# curl stops with "unsupported protocol" (exit 1); the last response is
# the redirect itself.
if [ "$CURL_EXIT" -eq 1 ] && [ "$HTTP_STATUS" -ge 300 ] && [ "$HTTP_STATUS" -lt 400 ]; then
    LOCATION=$(tr -d '\r' < "$HEADER_DUMP" \
        | awk 'tolower($1) == "location:" {print $2}' | tail -1)
    log "REDIRECT NOT FOLLOWED: $URI -> $LOCATION"
    if [ "$INLINE" = true ]; then
        send_transparent_gif
        exit 0
    fi
    ESC_URI=$(printf "%s" "$URI" | sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g; s/"/\&quot;/g')
    ESC_LOC=$(printf "%s" "$LOCATION" | sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g; s/"/\&quot;/g')
    printf "HTTP/1.0 200 OK\r\n"
    printf "Content-Type: text/html\r\n"
    printf "\r\n"
    printf "<html><head><title>Redirect to an unencrypted address</title></head><body>\n"
    printf "<h2>Redirect to an unencrypted address</h2>\n"
    printf "<p>The secure page<br><b>%s</b><br>redirects to an address without encryption:</p>\n" "$ESC_URI"
    printf "<p><a href=\"%s\">%s</a></p>\n" "$ESC_LOC" "$ESC_LOC"
    printf "<p>Follow the link only if you are content to continue without encryption:\n"
    printf "what you send and receive there can be read and changed on the way.</p>\n"
    printf "</body></html>\n"
    exit 0
fi

# --- A failed image request gets a transparent GIF instead of an error page ---
if [ "$CURL_EXIT" -ne 0 ] || [ "$HTTP_STATUS" -ge 400 ]; then
    if [ "$INLINE" = true ]; then
        log "RESCUE: image request failed, sending 1x1 GIF"
        send_transparent_gif
        exit 0
    fi
    case "$URI" in
        *.png*|*.gif*|*.jpg*|*.jpeg*|*.ico*)
            log "RESCUE: image request failed, sending 1x1 GIF"
            send_transparent_gif
            exit 0
            ;;
    esac
fi

# --- Pass the response on to Chimera with its real status code ---
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
