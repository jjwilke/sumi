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



noinst_LTLIBRARIES = libsumi_sst.la
libsumi_sst_la_LDFLAGS = 

libsumi_sst_la_SOURCES = \
  sumi_api.cc \
  sumi_transport.cc

library_includedir=$(includedir)/sumi/sst

nobase_library_include_HEADERS = \
 sumi_api.h \
 rdma.h \
 sumi_transport.h

# subscription.cc 
# subscription.h 


