# Common makefile -- loads make rules for each platform

BUILD=build
INT=integrations
ZT1=zerotierone

OSTYPE=$(shell uname -s)

ifeq ($(OSTYPE),Darwin)
	include make-mac.mk
endif

ifeq ($(OSTYPE),Linux)
	include make-linux.mk
endif

ifeq ($(OSTYPE),FreeBSD)
	include make-freebsd.mk
endif
ifeq ($(OSTYPE),OpenBSD)
	include make-freebsd.mk
endif
