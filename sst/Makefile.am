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

if ENABLE_SST_SUBREPO
#include parent SST stuff
AM_CPPFLAGS += -I$(top_srcdir)/.. -I$(top_builddir)/..
AM_CPPFLAGS += -I$(top_srcdir)/../sprockit -I$(top_builddir)/../sprockit
AM_CPPFLAGS += -I$(top_builddir)/../sstmac/replacements -I$(top_srcdir)/../sstmac/replacements 
AM_CPPFLAGS += -I$(top_srcdir)/../sstmac/install -DSSTMAC=1
endif



noinst_LTLIBRARIES = libdharma_sst.la
libdharma_sst_la_LDFLAGS = 

libdharma_sst_la_SOURCES = \
  dharma_api.cc \
  sumi_dharma_transport.cc

library_includedir=$(includedir)/dharma/sst

nobase_library_include_HEADERS = \
 dharma_api.h \
 rdma.h \
 sumi_dharma_transport.h

# subscription.cc 
# subscription.h 


