set (sst_HEADERS
sumi_dharma_transport.cc
dharma_api.cc
)

set (sst_SOURCES 
sumi_dharma_transport.h
dharma_api.h
)

include_directories( "${CMAKE_SOURCE_DIR}" )
add_library( sst OBJECT ${sst_SOURCES} ${sst_HEADERS} )

install (FILES rdma.h dharma_api.h DESTINATION include/sst)

