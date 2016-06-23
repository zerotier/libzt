#!/bin/bash
FILE="$1"
 
if ([ -f "$FILE" ] && [ -s "$FILE" ]) || ([ -d "$FILE" ] && [ "$(ls -A "$FILE")" ]);
then
   echo "[OK  ] $FILE"
else
   echo "[FAIL] $FILE" >&2
fi
