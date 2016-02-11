#ifndef mpi_RDMA_H
#define mpi_RDMA_H

#include <sumi/rdma_mdata.h>

namespace sumi {

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
