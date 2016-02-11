#
#   This file is part of SST/macroscale: 
#                The macroscale architecture simulator from the SST suite.
#   Copyright (c) 2009 Sandia Corporation.
#   This software is distributed under the BSD License.
#   Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#   the U.S. Government retains certain rights in this software.
#   For more information, see the LICENSE file in the top 
#   SST/macroscale directory.
#

AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir) 

noinst_LTLIBRARIES = libdharma_mpi.la
libdharma_mpi_la_LDFLAGS = 

libdharma_mpi_la_SOURCES = \
  mpi_transport.cc

library_includedir=$(includedir)/dharma/mpi

nobase_library_include_HEADERS = \
 mpi_transport.h \
 rdma.h

