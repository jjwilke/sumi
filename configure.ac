dnl
dnl   This file is part of SST/macroscale:
dnl                The macroscale architecture simulator from the SST suite.
dnl   Copyright (c) 2009 Sandia Corporation.
dnl   This software is distributed under the BSD License.
dnl   Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
dnl   the U.S. Government retains certain rights in this software.
dnl   For more information, see the LICENSE file in the top
dnl   SST/macroscale directory.
dnl

dnl Version info
m4_define([DHARMA_VERSION_TAG], 1)
m4_define([DHARMA_SUBVERSION_TAG], 0)
m4_define([DHARMA_SUBSUBVERSION_TAG], 0)
# Enable this for releases
dnl m4_define([DHARMA_SNAPSHOT_TAG])
# Enable this for development snapshots (should generally be enabled)
m4_define([DHARMA_ACVERSION_TAG],
          [DHARMA_VERSION_TAG.DHARMA_SUBVERSION_TAG.DHARMA_SUBSUBVERSION_TAG])

dnl Libtool library versioning is
dnl a useful idea implemented in a confusing way.
dnl -version-info A:B:C means:
dnl     Major version A
dnl     subversion B (backward compatible for all A:X where X <= B)
dnl     and A is backward compatible to major version A-C
dnl which means that C <= A,
dnl and C == 0 means no major-version backward compatibility.
m4_define([DHARMA_MAJOR_LIBVERS], 1)
m4_define([DHARMA_MINOR_LIBVERS], 0)
m4_define([DHARMA_BACKW_LIBVERS], 0)
m4_define([DHARMA_LIBVERSION_TAG],
          [DHARMA_MAJOR_LIBVERS:DHARMA_MINOR_LIBVERS:DHARMA_BACKW_LIBVERS])

AC_INIT([dharma], [DHARMA_ACVERSION_TAG], [jjwilke@sandia.gov])
AC_CONFIG_MACRO_DIR([acinclude])
AC_CONFIG_AUX_DIR(bin)
AC_CONFIG_HEADER(dharma/dharmaconfig-generated.h)
AX_PREFIX_CONFIG_H(dharma/dharma_config.h, DHARMA)

AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([-Wall -Werror -Wno-portability subdir-objects foreign])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_PROG_CXX
AC_PROG_LIBTOOL
AC_LANG_CPLUSPLUS
LT_INIT([disable-shared])


# Version info.
AH_TEMPLATE([DHARMA_VERSION], [Major version number])
AH_TEMPLATE([DHARMA_SUBVERSION], [Major version number])
AH_TEMPLATE([DHARMA_SUBSUBVERSION], [Major version number])
AC_DEFINE_UNQUOTED(DHARMA_VERSION, [DHARMA_VERSION_TAG])
AC_DEFINE_UNQUOTED(DHARMA_SUBVERSION, [DHARMA_SUBVERSION_TAG])
AC_DEFINE_UNQUOTED(DHARMA_SUBSUBVERSION, [DHARMA_SUBSUBVERSION_TAG])
AC_SUBST(DHARMA_LIBVERSION, [DHARMA_LIBVERSION_TAG])


AC_ARG_ENABLE(sst-subrepo,
  [AS_HELP_STRING([--(dis|en)able-sst-subrepo],
    [Whether building as a subrepo of SST])],
  [ enable_sst_subrepo=$enableval ], 
  [ enable_sst_subrepo=no ])

if test "X$enable_sst_subrepo" = "Xyes"; then
  AM_CONDITIONAL(ENABLE_SST_SUBREPO, true)
else
  AM_CONDITIONAL(ENABLE_SST_SUBREPO, true)
fi

CHECK_REPO_BUILD([dharma])

CHECK_CPP11()

CHECK_SST()

CHECK_MPI()

CHECK_GNI()

CHECK_SPINLOCK()

AC_CONFIG_FILES([
 Makefile
 dharma/Makefile
 test/Makefile
 dharma/config.h:config.h.in
])
AC_OUTPUT

echo "Configuration Summary:"
echo "---------------------------------------------------------------"
echo "Install prefix     $prefix"
echo "C compiler         $CC"
echo "C++ compiler       $CXX"
echo "CFLAGS             $CFLAGS"
echo "CXXFLAGS           $CXXFLAGS"
echo "CPPFLAGS           $CPPFLAGS"
echo "LDFLAGS            $LDFLAGS"
echo "SST Subrepo        $enable_sst_subrepo"
echo "SST Conduit        $enable_sst"
echo "MPI Conduit        $enable_mpi"
echo "GNI Conduit        $enable_gni"
