#!/bin/bash
FILE="$1"
 
if [ -f "$FILE" ]  || [ -d "$FILE" ];
then
   echo "[OK  ] $FILE"
else
   echo "[FAIL] $FILE" >&2
fi
