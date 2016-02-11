#ifndef GNI_RDMA_H
#define GNI_RDMA_H

#include <dharma/rdma_mdata.h>
#include <gni_pub.h>

namespace dharma {

struct public_buffer :
 public public_buffer_base
{
 public:
  explicit public_buffer(void* buf){
    ptr = buf;
  }

  public_buffer() {
    ptr = 0;
  }

  gni_mem_handle_t mem_handle;

};

}

#endif // GNI_RDMA_H
