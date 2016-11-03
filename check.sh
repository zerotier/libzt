#!/bin/bash
FILE="$1"
 
if ([ -f "$FILE" ] && [ -s "$FILE" ]) || ([ -d "$FILE" ] && [ "$(ls -A "$FILE")" ]);
then
   echo "[OK  ] $FILE"
   exit 0
else
   echo "[FAIL] $FILE" >&2
   exit 1
fi
