dnl -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(MUSCAL2, 1.1.0, software@scec.org)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign tar-pax no-exeext])

dnl
dnl Dependencies
dnl

dnl
dnl Command line tools
dnl

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_PROG_LN_S

dnl --- begin: add libtool and --enable-shared handling ---
# initialize libtool (provides LT_INIT and shared/static helpers)
# If libtool is not available, autoreconf will tell you.
LT_INIT

# standard --enable-shared/--disable-shared option
AC_ARG_ENABLE([shared],
  [AS_HELP_STRING([--enable-shared|--disable-shared],
    [build shared libraries (default: enabled)])],
  [case "${enableval}" in
     yes) enable_shared=yes ;;
     no) enable_shared=no ;;
     *) enable_shared=${enableval} ;;
   esac],
  [enable_shared=yes])

# export the variable for Makefile.in / Automake
AC_SUBST([enable_shared])
dnl --- end ---

dnl TAR
AC_PATH_PROG(TAR, tar)
if test -z "$TAR" ; then
  AC_MSG_FAILURE([cannot find 'tar' program.])
  TAR=`echo "Error: tar is not installed." ; false`
fi

dnl
dnl Setup environment so dependencies are used in build
dnl

CFLAGS="$CFLAGS"
CPPFLAGS="-I$prefix/include $CPPFLAGS"
LDFLAGS="-L$prefix/lib -L$prefix/lib64 $LDFLAGS"

dnl
dnl Verify configuration
dnl

##check optional large data path
##MUSCAL2_LARGEDATA_DIR=$CVM_LARGEDATA_DIR/model/muscal2
if test x"$CVM_LARGEDATA_DIR" != x; then
   # test directory existence
   MUSCAL2_LARGEDATA_DIR=$CVM_LARGEDATA_DIR/model/muscal2
# In docker container building.. this is not accessible  yet ???
   if test x"$CVM_IN_DOCKER" != x; then
     AM_CONDITIONAL([WITH_MUSCAL2_LARGEDATA_DIR], true)
     AC_SUBST(MUSCAL2_LARGEDATA_DIR)
   else 
     AC_CHECK_FILE([$MUSCAL2_LARGEDATA_DIR/model_MUSCAL_CANVAS_dll0.01_vardz_float32_cmpd.tiledb],
                [AM_CONDITIONAL([WITH_MUSCAL2_LARGEDATA_DIR], true) AC_SUBST(MUSCAL2_LARGEDATA_DIR)],
                [AM_CONDITIONAL([WITH_MUSCAL2_LARGEDATA_DIR], false)])
   fi
else
   AM_CONDITIONAL(WITH_MUSCAL2_LARGEDATA_DIR, false)
fi

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  data/Makefile
  test/Makefile
  ])

AC_OUTPUT

AM_CONDITIONAL([BUILD_SHARED_LIBRARY], [test "$enable_shared" = yes])
if test "$enable_shared" = yes; then
  AC_MSG_NOTICE([Building shared libraries. Shared libraries enabled.])
else
  AC_MSG_WARN([Building static libraries. Shared libraries are not enabled.])
fi


dnl End of file
