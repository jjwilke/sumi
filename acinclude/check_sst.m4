

AC_DEFUN([CHECK_SST], [

AC_ARG_ENABLE(sst,
  [AS_HELP_STRING(
    [--(dis|en)able-sst],
    [Whether to compile SST transport conduits]
    )],
  [
    enable_sst=$enableval
  ], [
    enable_sst=no
  ]
)

if test "X$enable_sst" = "Xyes"; then
  AM_CONDITIONAL(ENABLE_SST, true)
  AC_SUBST([rdma_header_file_include], ["<sst/rdma.h>"])
  AC_SUBST([default_transport], ["sst"])
  AC_CONFIG_FILES([sst/Makefile])
else
  AM_CONDITIONAL(ENABLE_SST, false)
fi

])

