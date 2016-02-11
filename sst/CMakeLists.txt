set (sst_HEADERS
sumi_sumi_transport.cc
sumi_api.cc
)

set (sst_SOURCES 
sumi_sumi_transport.h
sumi_api.h
)

include_directories( "${CMAKE_SOURCE_DIR}" )
add_library( sst OBJECT ${sst_SOURCES} ${sst_HEADERS} )

install (FILES rdma.h sumi_api.h DESTINATION include/sst)

