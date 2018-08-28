#!/bin/bash

rm -rf bin build products tmp
rm -f *.o *.s *.exp *.lib .depend* *.core core
rm -rf .depend
find . -type f \( -name '*.o' -o -name '*.o.d' -o -name \
	'*.out' -o -name '*.log' -o -name '*.dSYM' -o -name '*.class' \) -delete
