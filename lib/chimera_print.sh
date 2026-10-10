#!/bin/bash

# Input from chimera-dialog
INPUT_PATH="$1"

# If empty use standard name in $HOME
if [ -z "$INPUT_PATH" ]; then
    OUTPATH="$HOME/chimera_output.pdf"
else
    # remove .pdf - avoid .pdf.pdf
    CLEAN_PATH="${INPUT_PATH%.pdf}"

    # check if path starts with '/' (absolute path)
    if [[ "$CLEAN_PATH" == /* ]]; then
        OUTPATH="$CLEAN_PATH.pdf"
    else
        # relativ path or just plain filename save in $HOME
        OUTPATH="$HOME/$CLEAN_PATH.pdf"
    fi
fi

# create destination folder if not exist
mkdir -p "$(dirname "$OUTPATH")" 2>/dev/null

# create temp file to catch text from stdin
TMPFILE=$(mktemp)

# read data from stdin and save in temp-file
cat > "$TMPFILE"

# check if text2pdf is installed
if command -v text2pdf &> /dev/null; then
    # convert temp file to PDF at requested path
    text2pdf -I -A4 "$TMPFILE" > "$OUTPATH"
else
    # fallback to lpr if text2pdf is missing
    lpr -h -p "$TMPFILE"
fi

# clean
rm -f "$TMPFILE"
