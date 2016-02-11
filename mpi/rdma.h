#ifndef mpi_RDMA_H
#define mpi_RDMA_H

#include <dharma/rdma_mdata.h>

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

};

}

#endif // RDMA_H
