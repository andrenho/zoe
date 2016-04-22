#
# prelude
#
AC_INIT([zoe], [0.3.0], [https://github.com/andrenho/zoe/issues])
AC_CONFIG_SRCDIR([src/main.cc])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror dist-bzip2 subdir-objects])

#
# check for files
#
AC_REQUIRE_AUX_FILE([tap-driver.sh])

#
# check for programs
#
: ${CXXFLAGS=""}             	    # remove default silly CXXFLAGS
AC_PROG_CXX([g++ clang++])
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_AWK
AX_VALGRIND_DFLT([drd], [off])
AX_VALGRIND_DFLT([helgrind], [off])
AX_VALGRIND_CHECK

# gold
AC_CHECK_PROGS([gold], [ld.gold])      # look for gold linker (instead of ld)
AM_CONDITIONAL([GOLD], [test "$gold" = ld.gold])

# cloc
AC_CHECK_PROGS([cloc], [cloc], [false])   # look for cpplint
if test "$cloc" = false; then
  AC_MSG_WARN(['cloc' was not found in the path. The target 'cloc' won't be built.'])
fi
AM_CONDITIONAL([CLOC], [test "$cloc" = cloc])

# cpplit
AC_CHECK_PROGS([cpplint], [cpplint], [false])   # look for cpplint
if test "$cpplint" = false; then
  AC_MSG_WARN(['cpplint' was not found in the path. The target 'cpplint' won't be built.'])
fi
AM_CONDITIONAL([CPPLINT], [test "$cpplint" = cpplint])

#
# check for libraries
#
AX_LIB_READLINE

#
# check for header files
#

#
# check for typedefs, structures and compiler characteristics
#
AX_CXX_COMPILE_STDCXX_14([noext], [mandatory])
AM_CONDITIONAL([GCC], [test "$GXX" = yes])

#
# check for conditional options
#

# debug
AC_ARG_ENABLE([debug], 
	      AS_HELP_STRING([--enable-debug], [Enable debugging info]),
	      [debug=yes])
AM_CONDITIONAL([DEBUG], [test "$debug" = yes])

# profile
AC_ARG_ENABLE([profile], 
	      AS_HELP_STRING([--enable-profile], [Enable generation of profile info]),
	      [profile=yes])
AM_CONDITIONAL([PROFILE], [test "$profile" = yes])

# coverage
AC_ARG_ENABLE([coverage], 
	      AS_HELP_STRING([--enable-coverage], [Enable generation of coverage info]),
	      [coverage=yes])
AM_CONDITIONAL([COV], [test "$coverage" = yes])


#
# output files
#
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT