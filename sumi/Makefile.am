#
#   This file is part of SST/macroscale: 
#                The macroscale architecture simulator from the SST suite.
#   Copyright (c) 2011 Sandia Corporation.
#   This software is distributed under the BSD License.
#   Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#   the U.S. Government retains certain rights in this software.
#   For more information, see the LICENSE file in the top 
#   SST/macroscale directory.
#

AM_CPPFLAGS = -I$(top_builddir) -I$(top_srcdir) 

if ENABLE_SST_SUBREPO
noinst_LTLIBRARIES = libsumi.la
AM_CPPFLAGS += -I$(top_builddir)/.. -I$(top_srcdir)/..
AM_CPPFLAGS += -I$(top_srcdir)/../sprockit -I$(top_builddir)/../sprockit
AM_CPPFLAGS += -I$(top_srcdir)/../sstmac/install -DSSTMAC=1
AM_CPPFLAGS += -I$(top_builddir)/../sstmac/replacements -I$(top_srcdir)/../sstmac/replacements 
else
lib_LTLIBRARIES = libsumi.la
libsumi_la_LDFLAGS = -version-info @SUMI_LIBVERSION@
endif

libsumi_la_LIBADD = 

if ENABLE_MPI
libsumi_la_LIBADD += ../mpi/libsumi_mpi.la
endif

if ENABLE_GNI
libsumi_la_LIBADD += ../gni/libsumi_gni.la
endif

library_includedir=$(includedir)/sumi/sumi

nodist_library_include_HEADERS = config.h sumi_config.h

library_include_HEADERS = \
 active_msg_transport.h \
 allgather.h \
 allreduce.h \
 bcast.h \
 collective.h \
 collective_actor.h \
 collective_actor_fwd.h \
 collective_message.h \
 collective_message_fwd.h \
 comm_functions.h \
 dense_rank_map.h \
 domain.h \
 domain_fwd.h \
 dynamic_tree_vote.h \
 lockable.h \
 message.h \
 monitor.h \
 options.h \
 partner_timeout.h \
 ping.h \
 rdma.h \
 rdma_interface.h \
 rdma_mdata.h \
 thread.h \
 thread_lock.h \
 thread_safe_int.h \
 thread_safe_list.h \
 thread_safe_ptr_type.h \
 thread_safe_refcount_ptr.h \
 thread_safe_set.h \
 timeout.h \
 transport.h \
 transport_fwd.h 

libsumi_la_SOURCES = \
 active_msg_transport.cc \
 allgather.cc \
 allreduce.cc \
 bcast.cc \
 collective.cc \
 collective_actor.cc \
 collective_message.cc \
 dense_rank_map.cc \
 domain.cc \
 dynamic_tree_vote.cc \
 message.cc \
 monitor.cc \
 partner_timeout.cc \
 ping.cc \
 thread_lock.cc \
 thread_safe_set.cc \
 transport.cc


