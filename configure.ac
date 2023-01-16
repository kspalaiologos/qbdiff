AC_PREREQ([2.68])
AC_INIT([qbdiff], [m4_esyscmd(build-aux/git-version-gen .tarball-version)], [https://github.com/kspalaiologos/qbdiff])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign subdir-objects tar-ustar dist-bzip2 dist-xz dist-zip color-tests])
AM_SILENT_RULES([yes])
AC_CONFIG_MACRO_DIR([build-aux])

AC_PROG_CC([clang gcc icc])
AC_PROG_AWK
LT_INIT

PKG_PROG_PKG_CONFIG
PKG_INSTALLDIR

AC_C_RESTRICT
AC_OPENMP

AX_SUBST_TRANSFORMED_PACKAGE_NAME
AX_SUBST_MAN_DATE

AC_CONFIG_FILES([Makefile qbdiff.pc])
AC_CONFIG_FILES([man/qbdiff.1 man/qbpatch.1])
AC_ARG_PROGRAM
AC_OUTPUT
