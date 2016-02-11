#ifndef dharma_active_msg_transport_h
#define dharma_active_msg_transport_h

#include <dharma/monitor.h>
#include <dharma/timeout.h>
#include <dharma/message.h>
#include <dharma/collective.h>
#include <dharma/transport.h>
#include <dharma/comm_functions.h>

namespace dharma {

class active_msg_transport :
  public transport
{
 public:
  void
  cq_notify(){} //no op

  void
  delayed_transport_handle(const message::ptr &msg){
    handle(msg);
  }

  void
  schedule_ping_timeout(pinger *pnger, double to){}

  double
  wall_time() const;

  void
  schedule_next_heartbeat();

  virtual collective_done_message::ptr
  collective_block(collective::type_t ty, int tag);

  message::ptr
  block_until_message();

  message::ptr
  block_until_message(double timeout);

  void
  init();

  typedef enum {
   i_am_alive,
   i_am_dead
  } ping_status_t;

 protected:
  void
  maybe_do_heartbeat();

  active_msg_transport();

  char*
  allocate_message_buffer(const message::ptr& msg, int& size);

  message::ptr
  deserialize(char* buf);

  message::ptr
  free_message_buffer(void* buf);

  char* allocate_smsg_buffer();

  void free_smsg_buffer(void* buf);

 private:
  double time_zero_;

  double next_heartbeat_;

 protected:
  virtual void block_inner_loop() = 0;

  char* smsg_buffer_;

  const int smsg_buffer_size_;

  std::list<char*> smsg_buffer_pool_;
};

}

#endif
