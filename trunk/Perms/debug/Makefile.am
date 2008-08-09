

include ../flagsandlibs
include ../sources

permsdebug_SOURCES = $(COMMONSRC)

permsdebug_CFLAGS = -ggdb -O0

bin_PROGRAMS = permsdebug

install-exec-local:
	cd $(DESTDIR)$(bindir) && \
	rm permsdebug

