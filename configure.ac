AC_INIT([Kent C++CSP Library v2],[2.0.1e],[cppcsp@twistedsquare.com],[cppcsp2])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE(dist-bzip2)

AM_CONDITIONAL(MINGW,test x`echo $target_os | grep mingw` = x$target_os || test x`echo $target_os | grep cygwin` = x$target_os)
AM_CONDITIONAL(BSD,test x`echo $target_os | grep bsd` = x$target_os)
AM_CONDITIONAL(LINUX,test x`echo $target_os | grep linux` = x$target_os)
AM_CONDITIONAL(DARWIN,test x`echo $target_os | grep darwin` = x$target_os)
AM_CONDITIONAL(X86,test x`echo $target_arch | grep .86` = x$target_arch)

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

AC_COMPUTE_INT(NCCB_SIZEOF_VOIDP,SIZEOF_VOIDP)

AC_SUBST(NCCB_SIZEOF_VOIDP)

AC_OUTPUT(Makefile)

