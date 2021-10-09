#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([ilbtools], [1.0], [jondaniel879@gmail.com])
AC_COPYRIGHT([(C) 2021 Jon Daniel <jondaniel879@gmail.com>])
AC_CONFIG_SRCDIR([src/ilb2png.cpp])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX
AC_PROG_MAKE_SET
AM_MAINTAINER_MODE
# Checks for libraries.
AC_CHECK_HEADERS([stdlib.h stdio.h stdint.h string.h error.h errno.h sys/stat.h unistd.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
