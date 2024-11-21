# Guess the $PREFIX
.ifndef PREFIX
PREFIX!=		sysctl -n user.localbase
.endif
PREFIX?=		/usr/local

# Infer $BINDIR and DEBUGFILEDIR.
BINDIR?=		$(PREFIX)/bin
DEBUGFILEDIR?=		$(PREFIX)/lib/debug/bin
SHAREDIR?=              $(PREFIX)/share

# Newer C standards aren't supported by the system compiler on FreeBSD 14.1.
CSTD=			c17

# Use libxo(3) for (optionally) structured output.
LDADD+=			-lxo

PROG=			ethers
SRCS+=			allocator.c cli_args.c scan.c ethers_file.c main.c

# Enable draconic compiler errors for debug builds.
.if !empty(.TARGETS:tw:Mdebug)
CFLAGS=			-O0 -g -pipe
CFLAGS+=		-DRACONIC
.endif

debug: clean .WAIT $(PROG)

.PHONY: xolint
xolint: $(SRCS)
	+xolint $(SRCS)

allocator.o: allocator.h allocator.c
cli_args.o: slice.h scan.h cli_args.h cli_args.c
scan.o: slice.h scan.h scan.c
ethers_file.o: cli_args.h scan.h slice.h ethers_file.h ethers_file.c
main.o: cli_args.h ethers_file.h scan.h slice.h main.c

.include <bsd.prog.mk>

.MAKE.MODE=meta
