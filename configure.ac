#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([fdk-aac], [0.1.0], [http://sourceforge.net/projects/opencore-amr/])
AC_CONFIG_AUX_DIR(.)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([tar-ustar])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_HEADERS([config.h])

# Cross compiling support
AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CXX

# Setup for libtool
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

# soname version to use
# goes by ‘current[:revision[:age]]’ with the soname ending up as
# current.age.revision.
FDK_AAC_VERSION=0:1:0
AC_SUBST(FDK_AAC_VERSION)

AC_CONFIG_FILES([Makefile
                 fdk-aac.pc])
AC_OUTPUT
