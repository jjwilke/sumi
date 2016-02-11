#ifndef sstmac_sw_api_simpsg_ALLGATHER_H
#define sstmac_sw_api_simpsg_ALLGATHER_H

#include <dharma/collective.h>
#include <dharma/collective_actor.h>
#include <dharma/collective_message.h>
#include <dharma/comm_functions.h>

DeclareDebugSlot(dharma_allgather)

namespace dharma {

class bruck_actor :
  public dag_collective_actor
{

 public:
  std::string
  to_string() const {
    return "bruck actor";
  }

 protected:
  void finalize();

  void finalize_buffers();
  void init_buffers(void *dst, void *src);
  void init_dag();

  void buffer_action(void *dst_buffer, void *msg_buffer, action* ac);

  void dense_partner_ping_failed(int dense_rank){
    dag_collective_actor::dense_partner_ping_failed(dense_rank);
  }


};

class bruck_collective :
  public dag_collective
{

 public:
  std::string
  to_string() const {
    return "allgather";
  }

  dag_collective_actor*
  new_actor() const {
    return new bruck_actor;
  }

  dag_collective*
  clone() const {
    return new bruck_collective;
  }

};

}

#endif // ALLGATHER_H
