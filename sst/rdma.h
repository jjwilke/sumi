#ifndef sst_RDMA_H
#define sst_RDMA_H

#include <dharma/rdma_mdata.h>

namespace dharma {

struct public_buffer : public public_buffer_base
{
  public_buffer(void* buf){
    ptr = buf;
  }

  public_buffer(){
    ptr = 0;
  }

  void offset_ptr(int offset) {
    if (ptr){
      public_buffer_base::offset_ptr(offset);
    }
  }
};

}

#endif // RDMA_H
