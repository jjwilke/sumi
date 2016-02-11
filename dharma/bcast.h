#ifndef dharma_bcast_included_h
#define dharma_bcast_included_h

#include <dharma/collective.h>
#include <dharma/collective_actor.h>
#include <dharma/collective_message.h>
#include <dharma/comm_functions.h>

DeclareDebugSlot(dharma_bcast)

namespace dharma {

class binary_tree_bcast_actor :
  public dag_collective_actor
{
 public:
  std::string
  to_string() const {
    return "bcast actor";
  }

  ~binary_tree_bcast_actor(){}

 private:
  void finalize_buffers();
  void init_buffers(void *dst, void *src);
  void init_dag();

  void init_root(int me, int roundNproc, int nproc);
  void init_child(int me, int roundNproc, int nproc);
  void init_internal(int me, int windowSize, int nproc, action* recv);
  void buffer_action(void *dst_buffer, void *msg_buffer, action *ac);
};

class binary_tree_bcast_collective :
  public dag_collective
{

 public:
  std::string
  to_string() const {
    return "bcast";
  }

  dag_collective_actor*
  new_actor() const {
    return new binary_tree_bcast_actor;
  }

  dag_collective*
  clone() const {
    return new binary_tree_bcast_collective;
  }

};

}

#endif // BCAST_H
