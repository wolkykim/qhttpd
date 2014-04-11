################################################################################
## qHttpd - http://www.qdecoder.org
##
## Copyright (c) 2008-2012 Seungyoung Kim.
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## 1. Redistributions of source code must retain the above copyright notice,
##    this list of conditions and the following disclaimer.
## 2. Redistributions in binary form must reproduce the above copyright notice,
##    this list of conditions and the following disclaimer in the documentation
##    and/or other materials provided with the distribution.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
################################################################################
## $Id: configure.ac 214 2012-05-05 00:33:31Z seungyoung.kim $
################################################################################

AC_COPYRIGHT([
=============================================================================
 qHttpd - http://www.qdecoder.org

 Copyright (c) 2008-2012 Seungyoung Kim.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
=============================================================================
])

## Internal functions
AC_DEFUN([Q_ARG_ENABLE], [
	AC_ARG_ENABLE([$1],[AS_HELP_STRING([--enable-$1], [$2])],,[enableval=no])
	if test "$enableval" = yes; then
		AC_MSG_NOTICE(['$1' feature is enabled])
		CPPFLAGS="$CPPFLAGS $3"
		OPT_OBJS="$OPT_OBJS $4"
	fi
])

AC_DEFUN([Q_ARG_DISABLE], [
	AC_ARG_ENABLE([$1],[AS_HELP_STRING([--disable-$1], [$2])],,[enableval=yes])
	if test "$enableval" = no; then
		AC_MSG_NOTICE(['$1' feature is disabled])
		CPPFLAGS="$CPPFLAGS $3"
	fi
])

## Initialize
AC_INIT([qHttpd], [1 RELEASE], [http://www.qdecoder.org/])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile lib/Makefile src/Makefile])

## Set path
PATH="$PATH:/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin"
CPPFLAGS="$CPPFLAGS -I./ -I/usr/include -I/usr/local/include"
CPPFLAGS="$CPPFLAGS -D_GNU_SOURCE"
CPPFLAGS="$CPPFLAGS -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64"
LDFLAGS="$LDFLAGS -L/usr/lib -L/usr/local/lib"

## Set autoconf setting
#AC_CANONICAL_TARGET
AC_PREFIX_DEFAULT([/usr/local/qhttpd])
#AC_PRESERVE_HELP_ORDER

## Checks for programs.
AC_PROG_CC
if test $ac_compiler_gnu = yes; then
	CFLAGS="-Wall"
#else
#	AC_MSG_FAILURE([GCC is required.])
fi
AC_PROG_CC_C99
if test $ac_cv_prog_cc_c99 = no; then
        AC_MSG_FAILURE([Compiler does not support C99 mode.])
fi

AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PATH_PROG(RM, rm)

## Checks for header files.
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_HEADER_STAT
AC_HEADER_DIRENT
AC_CHECK_HEADER([inttypes.h])

## Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_OFF_T

## Checks for library functions.
#AC_CHECK_FUNCS([socket sendfile])

## Set additional arguments
OPT_OBJS=""

Q_ARG_ENABLE([lua], [enable LUA script engine], [-DENABLE_LUA], [luascript.o])
if test "$enableval" = yes; then
	AC_CHECK_HEADER([lua.h], [], AC_MSG_ERROR([Lua library 5.1.4 or above must be installed.]))
	AC_CHECK_LIB([lua], [main], [], AC_MSG_ERROR([Lua library 5.1.4 or above must be installed to enable Lua Script feature.]))
	AC_CHECK_LIB([dl], [main])
	AC_CHECK_LIB([m], [main])
fi

Q_ARG_ENABLE([hook], [enable hook support - compile with hook.c], [-DENABLE_HOOK], [hook.o])

Q_ARG_DISABLE([debug], [disable debug build - take off all debugging codes])
if test "$enableval" = yes; then
	CFLAGS="$CFLAGS -g"
	CPPFLAGS="$CPPFLAGS -DENABLE_DEBUG"
fi

## Print out setting
AC_MSG_NOTICE([CFLAGS $CFLAGS])
AC_MSG_NOTICE([CPPFLAGS $CPPFLAGS])
AC_MSG_NOTICE([LDFLAGS $LDFLAGS])
AC_MSG_NOTICE([LIBS $LIBS $OPT_OBJS])

## Create Makefile
AC_SUBST([OPT_OBJS], [$OPT_OBJS] )
AC_OUTPUT
