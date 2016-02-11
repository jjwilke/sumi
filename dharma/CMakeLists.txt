set (dharma_api_HEADERS
lockable.h
thread_lock.h
thread_safe_int.h
thread_safe_list.h
thread_safe_ptr_type.h
thread_safe_refcount_ptr.h
thread_safe_set.h
)

set (dharma_api_SOURCES 
thread_lock.cc
thread_safe_set.cc
)

if (NOT NO_TRANSPORT)

set (dharma_api_HEADERS
${dharma_api_HEADERS}
active_msg_transport.h
allgather.h
allreduce.h
bcast.h 
collective.h
collective_actor.h
collective_message.h
collective_message_fwd.h
comm_functions.h
dense_rank_map.h
domain.h
domain_fwd.h
dynamic_tree_vote.h
lockable.h
message.h
monitor.h
options.h
partner_timeout.h
ping.h
rdma.h
rdma_mdata.h
timeout.h
transport.h
transport_fwd.h
)

set (dharma_api_SOURCES 
${dharma_api_SOURCES}
active_msg_transport.cc
allgather.cc
allreduce.cc
bcast.cc
collective.cc
collective_actor.cc
collective_message.cc
dense_rank_map.cc
domain.cc
dynamic_tree_vote.cc
message.cc
monitor.cc
partner_timeout.cc
ping.cc
rdma.cc
transport.cc
)

endif()

if (CRAPPY_OLD_CMAKE)
add_library(dharma_api SHARED ${dharma_api_SOURCES} ${dharma_api_HEADERS})

target_link_libraries( dharma_api dharma_gni )
else()
add_library(core OBJECT ${dharma_api_SOURCES} ${dharma_api_HEADERS} )

install (FILES ${dharma_api_HEADERS} ${CMAKE_BINARY_DIR}/dharma/config.h DESTINATION include/dharma)

set(extra_targets $<TARGET_OBJECTS:core>)
set(extra_libs sprockit)

if (NOT NO_TRANSPORT)
set(extra_targets ${extra_targets} $<TARGET_OBJECTS:fake>)
endif()

if (SST)
set(extra_targets ${extra_targets} $<TARGET_OBJECTS:sst>)
set(extra_libs ${extra_libs} sstmac)
endif()

if (MPI)
set(extra_targets ${extra_targets} $<TARGET_OBJECTS:mpi>)
endif()

if (GNI)
set(extra_targets ${extra_targets} $<TARGET_OBJECTS:gni>)
endif()

add_library(dharma_api SHARED ${extra_targets})

target_link_libraries( dharma_api ${extra_libs} )
endif()

install (TARGETS dharma_api DESTINATION lib)
