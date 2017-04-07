#!/bin/bash

red=`tput setaf 1`
green=`tput setaf 2`
reset=`tput sgr0`

FILE="$1"
 
if ([ -f "$FILE" ] && [ -s "$FILE" ]) || ([ -d "$FILE" ] && [ "$(ls -A "$FILE")" ]);
then
   echo "${green}[OK  ]${reset} $FILE"
   exit 0
else
   echo "${red}[FAIL]${reset} $FILE" >&2
   exit 1
fi
