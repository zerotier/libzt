#
# ZeroTier SDK - Network Virtualization Everywhere
# Copyright (C) 2011-2017  ZeroTier, Inc.  https://www.zerotier.com/
# 
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
# 
#  --
# 
#  You can be released from the requirements of the license by purchasing
#  a commercial license. Buying such a license is mandatory as soon as you
#  develop commercial closed-source software that incorporates or links
#  directly against ZeroTier software without disclosing the source code
#  of your own application.
# 

# NOTE: This file only exists as a convenience for cleaning. To build, use 
# CMake. Instructions are given in README.md

.PHONY: install
install:
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	mkdir -p $(DESTDIR)$(PREFIX)/include
	cp $(BUILDPATH)/$(STATIC_LIB) $(DESTDIR)$(PREFIX)/lib/
	cp include/libzt.h $(DESTDIR)$(PREFIX)/include/

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/*.a
	rm -f $(DESTDIR)$(PREFIX)/include/*.h

##############################################################################
## Misc                                                                     ##
##############################################################################

.PHONY: clean
clean:
	-rm rf bin build
	-rm f *.o *.s *.exp *.lib .depend* *.core core
	-rm -rf .depend
	-find . -type f \( -name '*.a' -o -name '*.o' -o -name '*.so' -o -name \
		'*.o.d' -o -name '*.out' -o -name '*.log' -o -name '*.dSYM' \) -delete	

