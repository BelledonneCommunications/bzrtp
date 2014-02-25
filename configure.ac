#   -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.



AC_INIT([bzrtp],[0.1])
AC_PREREQ(2.63)
AC_CONFIG_SRCDIR([src/bzrtp.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([tar-ustar --warnings=no-portability])
AC_PROG_LIBTOOL
AC_PROG_CC
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_ARG_ENABLE(strict,
[  --enable-strict       Enable error on compilation warning [default=yes]],
[wall_werror=$enableval],
[wall_werror=yes]
)
# configure option to disable the tests
PKG_CHECK_MODULES(CUNIT, cunit, [found_cunit=yes],[found_cunit=no])
if test "$found_cunit" = "no" ; then
	AC_CHECK_HEADERS(CUnit/CUnit.h,
		[
			AC_CHECK_LIB(cunit,CU_add_suite,[
				found_cunit=yes
				CUNIT_LIBS+=" -lcunit"
			])
			
		])
fi

if test "$found_cunit" = "no" ; then
	AC_MSG_WARN([Could not find cunit framework, tests are not compiled.])
else 
	AC_CHECK_LIB(cunit,CU_get_suite,[
					AC_DEFINE(HAVE_CU_GET_SUITE,1,[defined when CU_get_suite is available])
					],[foo=bar],[$CUNIT_LIBS])
	AC_CHECK_LIB(cunit,CU_curses_run_tests,[
					AC_DEFINE(HAVE_CU_CURSES,1,[defined when CU_curses_run_tests is available])
					],[foo=bar],[$CUNIT_LIBS])
fi
AC_ARG_ENABLE(tests,
        [AS_HELP_STRING([--disable-tests], [Disable compilation of tests])],
        [case "${enableval}" in
                yes)    tests_enabled=true ;;
                no)     tests_enabled=false ;;
                *)      AC_MSG_ERROR(bad value ${enableval} for --disable-tests) ;;
        esac],
        [tests_enabled=yes]
)
AM_CONDITIONAL(ENABLE_TESTS, test x$tests_enabled = xyes && test x$found_cunit = xyes)

CFLAGS="$CFLAGS -Wall -std=c89"

if test $GCC = yes && test $wall_werror = yes;  then
	CFLAGS="$CFLAGS -Werror "
fi

LIBS="$LIBS -lpolarssl "

# Create the following files from their .in counterparts
AC_CONFIG_FILES([
  Makefile
  src/Makefile 
  include/Makefile 
  include/bzrtp/Makefile 
  test/Makefile 
  libbzrtp.pc
])

AC_OUTPUT