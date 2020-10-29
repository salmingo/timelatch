dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(timelatch, 1.0)


AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE()

AC_PROG_CXX
AC_PROG_CC
CXXFLAGS="-std=c++0x"

AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],
               [enable debugging, default: no]),
[case "${enableval}" in
             yes) debug=true ;;
             no)  debug=false ;;
             *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],
[debug=false])

AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

AC_TYPE_UINT32_T
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T
AC_CHECK_HEADER_STDBOOL
AC_CHECK_HEADERS([sys/param.h])
AC_CHECK_FUNCS([getcwd mkdir memset])

AC_CONFIG_FILES(Makefile src/Makefile)
AC_OUTPUT
