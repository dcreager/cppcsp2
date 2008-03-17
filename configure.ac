AC_INIT([Kent C++CSP Library v2],[2.0.3],[cppcsp@twistedsquare.com],[cppcsp2])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE(dist-bzip2)

#TODO shift all these checks into a case statement

OS_MINOR=CPPCSP_OS_UNKNOWN

AC_MSG_CHECKING([for MinGW])
if test x`echo $target_os | grep mingw` = x$target_os || test x`echo $target_os | grep cygwin` = x$target_os; then
	AC_MSG_RESULT([yes])
	OS_MINGW=true
	OS_MINOR=CPPCSP_MINGW
else
	AC_MSG_RESULT([no])
	OS_MINGW=false
fi

AC_MSG_CHECKING([for BSD])
if test x`echo $target_os | grep bsd` = x$target_os; then
	AC_MSG_RESULT([yes])
	OS_BSD=true
	OS_MINOR=CPPCSP_BSD
else
	AC_MSG_RESULT([no])
	OS_BSD=false
fi

AC_MSG_CHECKING([for Linux])
if test x`echo $target_os | grep linux` = x$target_os; then
	AC_MSG_RESULT([yes])
	OS_LINUX=true
	OS_MINOR=CPPCSP_LINUX
else
	AC_MSG_RESULT([no])
	OS_LINUX=false
fi


AC_MSG_CHECKING([for Darwin])
if test x`echo $target_os | grep darwin` = x$target_os; then
	AC_MSG_RESULT([yes])
	OS_DARWIN=true
	OS_MINOR=CPPCSP_DARWIN
else
	AC_MSG_RESULT([no])
	OS_DARWIN=false
fi

AC_MSG_CHECKING([for x86])
if test x`echo $target_arch | grep .86` = x$target_arch; then
	AC_MSG_RESULT([yes])
	CPU_X86=true
else
	AC_MSG_ERROR([no -- sorry, currently only x86 (and x86_64) CPUs are supported])
	CPU_X86=false
fi

AC_CONFIG_SRCDIR(src/kernel.h)
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_RANLIB

AC_C_CONST
AC_C_INLINE
AC_HEADER_STDC

AC_CHECK_HEADER(boost/version.hpp,,AC_MSG_ERROR(C++ Boost Libraries not found - please download from http://www.boost.org/.  Please read the C++CSP2 documentation on installing them first, to potentially save yourself some time and effort))

#TODO check boost's version

AC_CHECK_SIZEOF(void*)
AC_COMPUTE_INT(CALC_SIZEOF_VOIDP,SIZEOF_VOIDP)

if test x`echo $CPU_X86` = xtrue; then
	CONFIG_CPU_FLAGS="#define CPPCSP_X86"
else
	CONFIG_CPU_FLAGS=""
fi

CPPCSP_LINK_LIBS=""

if test x`echo $OS_MINGW` = xtrue; then
	OS_MAJOR="
#ifndef WIN32
#define WIN32
#endif
"
	SCHED_TYPE=""
	CPPCSP_LINK_LIBS="-lws2_32 -lwsock32 -lwinmm"
else
	OS_MAJOR="
#define CPPCSP_POSIX
"
	SCHED_TYPE="
#define CPPCSP_LONGJMP
"
	if test x`echo $OS_BSD` = xtrue; then
		CPPCSP_LINK_LIBS="-lc_r"
	else
		if test x`echo $OS_DARWIN` = xtrue; then
			CPPCSP_LINK_LIBS="-lrt -lpthread"
		else
			CPPCSP_LINK_LIBS="-lrt -pthread"
		fi
	fi
fi

OS_TYPE="
$OS_MAJOR
#define $OS_MINOR
"

AC_SUBST(OS_TYPE)
AC_SUBST(CALC_SIZEOF_VOIDP)
AC_SUBST(SCHED_TYPE)
AC_SUBST(CONFIG_CPU_FLAGS)
AC_SUBST(PACKAGE_VERSION)

AC_SUBST(CPPCSP_LINK_LIBS)

AC_CONFIG_FILES(Makefile cppcsp2-2.0.pc src/cppcsp_config.h)

AC_OUTPUT
