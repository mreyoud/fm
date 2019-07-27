# see LICENSE file for copyright and license details.

VERSION = 0.0

PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

CPPFLAGS := -D_DEFAULT_SOURCE -DVERSION=\"${VERSION}\"
CFLAGS   := -std=c99 -pedantic -Wall -Wextra -Werror
LDFLAGS  := -s -lcurses -l0

CC = cc

