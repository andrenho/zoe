# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# add debugging information
DEBUG = no

# compiler and linker
CC = gcc

# warnings
WARNINGS = -Wpedantic -Wall -Wextra -Wshadow -Wstrict-overflow \
	   -fno-strict-aliasing -Wpointer-arith -Wcast-qual \
	   -Wstrict-prototypes -Wmissing-prototypes -Wfloat-equal \
	   -Wno-missing-field-initializers -Wformat=2 -Wcast-align \
	   -Wbad-function-cast -Wstrict-overflow=5 -Winline \
	   -Wundef -Wnested-externs -Wunreachable-code -Wlogical-op \
	   -Wfloat-equal -Wstrict-aliasing=3 -Wredundant-decls \
	   -Wold-style-definition -Wno-format-nonliteral \
	   -Wno-missing-braces
