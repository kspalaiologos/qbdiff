ACLOCAL_AMFLAGS = -I build-aux

AM_CFLAGS = -I$(top_srcdir)/include $(OPENMP_CFLAGS)

EXTRA_DIST = LICENSE 3rdparty/libsais-LICENSE README.md build-aux/git-version-gen

pkgconfig_DATA = qbdiff.pc

include_HEADERS = include/libqbdiff.h
noinst_HEADERS = include/libqbdiff_private.h include/libsais.h include/libsais64.h include/blake2b.h

lib_LTLIBRARIES = libqbdiff.la
libqbdiff_la_SOURCES = src/libqbdiff.c src/libsais.c src/libsais64.c src/blake2b.c
libqbdiff_la_LDFLAGS = -no-undefined -version-info 0:0:0

bin_PROGRAMS = qbdiff qbpatch
qbdiff_CFLAGS = $(AM_CFLAGS)
qbdiff_SOURCES = src/qbdiff.c
qbpatch_CFLAGS = $(AM_CFLAGS)
qbpatch_SOURCES = src/qbpatch.c
qbdiff_LDADD = libqbdiff.la
qbpatch_LDADD = libqbdiff.la

dist_man_MANS = man/qbdiff.1 man/qbpatch.1

CLEANFILES = $(bin_PROGRAMS)

# End standard generic autotools stuff

# Begin special handling for autoconf VERSION being updated on commit

BUILT_SOURCES = .version
CLEANFILES += $(BUILT_SOURCES) .version-prev

_BRANCH_REF != $(AWK) '{print ".git/" $$2}' .git/HEAD 2>/dev/null ||:

.version: $(_BRANCH_REF)
	@if [ -e "$(srcdir)/.tarball-version" ]; then \
		printf "$(VERSION)" > $@; \
	else \
		touch "$@-prev"; \
		if [ -e "$@" ]; then \
			cp "$@" "$@-prev"; \
		fi; \
		./build-aux/git-version-gen "$(srcdir)/.tarball-version" > $@; \
		cmp -s "$@" "$@-prev" || autoreconf configure.ac --force; \
	fi

dist-hook:
	printf "$(VERSION)" > "$(distdir)/.tarball-version"

# Begin developer convenience targets

.PHONY: format
format: $(qbdiff_SOURCES) $(qbpatch_SOURCES) $(libqbdiff_la_SOURCES) $(include_HEADERS) $(noinst_HEADERS)
	clang-format -i $^

.PHONY: cloc
cloc: $(qbdiff_SOURCES) $(qbpatch_SOURCES) $(libqbdiff_la_SOURCES) $(include_HEADERS) $(noinst_HEADERS)
	cloc $^
