#ifndef sstmac_sw_api_simpsg_ALLREDUCE_H
#define sstmac_sw_api_simpsg_ALLREDUCE_H

#include <dharma/collective.h>
#include <dharma/collective_actor.h>
#include <dharma/collective_message.h>
#include <dharma/comm_functions.h>

DeclareDebugSlot(dharma_allreduce)

namespace dharma {

class wilke_allreduce_actor :
  public dag_collective_actor
{

 public:
  std::string
  to_string() const {
    return "virtual all reduce actor";
  }

  void
  buffer_action(void *dst_buffer, void *msg_buffer, action* ac);

  wilke_allreduce_actor(reduce_fxn fxn);

 private:
  bool is_lower_partner(int virtual_me, int partner_gap);
  void finalize_buffers();
  void init_buffers(void *dst, void *src);
  void init_dag();

 private:
  reduce_fxn fxn_;

  int num_reducing_rounds_;

  int num_total_rounds_;

};

class wilke_halving_allreduce :
  public dag_collective
{
 public:
  std::string
  to_string() const {
    return "dharma allreduce";
  }

  wilke_halving_allreduce(reduce_fxn fxn);

  wilke_halving_allreduce(){}

  virtual void
  init_reduce(reduce_fxn fxn){
    fxn_ = fxn;
  }

  dag_collective_actor*
  new_actor() const {
    return new wilke_allreduce_actor(fxn_);
  }

  dag_collective*
  clone() const {
    return new wilke_halving_allreduce(fxn_);
  }

 private:
  reduce_fxn fxn_;

};

}

#endif // ALLREDUCE_H
