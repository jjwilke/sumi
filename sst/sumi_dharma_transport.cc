#include <sst/sumi_dharma_transport.h>
#include <sstmac/software/process/api.h>
#include <sstmac/software/process/app.h>
//#include <sstmac/software/process/operating_system.h>

using namespace sstmac;
using namespace sstmac::sw;
using namespace sprockit::dbg;

ImplementAPI(dharma, sumi_dharma_transport, "dharma");

namespace dharma {

SpktRegister("sst", transport, sumi_dharma_transport,
            "Create a DHARMA transport suitable for SST/macro");

sumi_dharma_transport::sumi_dharma_transport()
{
}

sstmac::sumi::transport_message::payload_ptr
sumi_dharma_transport::handle(const sstmac::sumi::transport_message::ptr& smsg)
{
  if (!smsg){
    //this is sloppy - but oh well
    //a null message is sent to me to signal that I have stuff waiting in my completion queue
    bool empty;
    debug_printf(sprockit::dbg::dharma, "Rank %d got cq notification", sumi_api::rank_);
    message::ptr next = completion_queue_.pop_front_and_return(empty);
    if (empty){
      spkt_throw(sprockit::value_error,
        "received null message, but completion queue is empty");
    }
    return next;
  }

  message::ptr my_msg = ptr_safe_cast(message, smsg->payload());
  switch (smsg->type())
  {
   //depending on the type, we might have to mutate the incoming message
   case hw::network_message::failure_notification:
    my_msg->set_payload_type(message::failure);
    transport::handle(my_msg);
    break;
   case hw::network_message::payload:
   {
    if (smsg->buffer() && my_msg->payload_type() == message::eager_payload){
      my_msg->eager_buffer() = smsg->buffer();
    }
    transport::handle(my_msg);
    break;
   }
   case hw::network_message::rdma_get_payload:
    smsg->complete_transfer(my_msg->local_buffer());
    if (my_msg->needs_recv_ack()) //only if I requested to be notified
      transport::handle(my_msg);
    break;
   case hw::network_message::rdma_put_payload:
    smsg->complete_transfer(my_msg->remote_buffer());
    if (my_msg->needs_recv_ack()) //only if I requested to be notified
      transport::handle(my_msg);
    break;
   case hw::network_message::rdma_get_nack:
    my_msg->set_payload_type(message::rdma_get_nack);
    transport::handle(my_msg);
    break;
   case hw::network_message::rdma_get_sent_ack:
    my_msg->set_payload_type(message::rdma_get_ack);
    transport::handle(my_msg);
    break;
   case hw::network_message::rdma_put_sent_ack:
    my_msg->set_payload_type(message::rdma_put_ack);
    transport::handle(my_msg);
    break;
   default:
    transport::handle(my_msg);
    break; //do nothing
  }

  bool empty;
  message::ptr msg = completion_queue_.pop_front_and_return(empty);
  return msg;  // will return message::ptr() if empty
}

void
sumi_dharma_transport::init()
{
  sstmac::sumi::sumi_api::init();
  transport::nproc_ = sstmac::sumi::sumi_api::nproc_;
  transport::rank_ = sstmac::sumi::sumi_api::rank_;
  transport::init();
}

void
sumi_dharma_transport::finalize()
{
  debug_printf(sprockit::dbg::dharma,
    "Rank %d finalizing", transport::rank_);
  sstmac::sumi::sumi_api::finalize();
  transport::finalize();
  monitor_->validate_done();
  stop_heartbeat();
  //sstmac_usleep(heartbeat_interval_*1e6);
  delete monitor_;

}

void
sumi_dharma_transport::go_die()
{
  kill_node();
}

void
sumi_dharma_transport::go_revive()
{
  spkt_throw(sprockit::illformed_error,
    "SST cannot revive a dead process currently");
}

void
sumi_dharma_transport::init_factory_params(sprockit::sim_parameters* params)
{
  //have to init this as an api, too
  sstmac::sw::thread* thr = sstmac::sw::thread::current();
  sstmac::sw::app_id aid = thr->aid();
  sstmac::sw::task_id tid = thr->tid();
  sstmac::sw::software_id sid(aid, tid);
  init_param1(sid);
  sstmac::sumi::sumi_api::init_factory_params(params);
  transport::init_factory_params(params);
}

void
sumi_dharma_transport::finalize_init()
{
  thread::current()->register_lib(this);
  sstmac::sumi::sumi_api::finalize_init();
}

void
sumi_dharma_transport::do_smsg_send(int dst, const message::ptr &msg)
{
  transport_send(msg->byte_length(), msg,
    sstmac::hw::network_message::payload,
    dst, msg->needs_send_ack(), msg->eager_buffer());
}

double
sumi_dharma_transport::wall_time() const
{
  return now().sec();
}

void
sumi_dharma_transport::do_send_ping_request(int dst)
{
  rdma_message::ptr msg = new rdma_message;
  msg->set_class_type(message::ping);
  //here, a simple rdma get
  rdma_get(dst, msg);
}

void
sumi_dharma_transport::do_rdma_get(int dst, const message::ptr& msg)
{
  transport_send(msg->byte_length(), msg,
    sstmac::hw::network_message::rdma_get_request,
    dst, msg->needs_send_ack(), msg->remote_buffer());
}

void
sumi_dharma_transport::do_rdma_put(int dst, const message::ptr& msg)
{
  transport_send(msg->byte_length(), msg,
    sstmac::hw::network_message::rdma_put_payload,
    dst, msg->needs_send_ack(), msg->local_buffer());
}

void
sumi_dharma_transport::do_nvram_get(int dst, const message::ptr& msg)
{
  transport_send(msg->byte_length(), msg,
    sstmac::hw::network_message::nvram_get_request,
    dst, msg->needs_send_ack(), msg->remote_buffer());
}

message::ptr
sumi_dharma_transport::block_until_message()
{
  sstmac::sumi::transport_message::payload_ptr msg = sstmac::sumi::sumi_api::poll_until_notification();
  return ptr_safe_cast(dharma::message, msg);
}

message::ptr
sumi_dharma_transport::block_until_message(double timeout)
{
  timestamp to(timeout);
  sstmac::sumi::transport_message::payload_ptr msg = sstmac::sumi::sumi_api::poll_until_notification(to);
  return ptr_test_cast(dharma::message, msg);
}

void
sumi_dharma_transport::cq_notify()
{
  debug_printf(sprockit::dbg::dharma, "Rank %d starting cq notification", sumi_api::rank_);
  //a null message indicates a cq notification
  if (blocked()){
    debug_printf(sprockit::dbg::dharma, "Rank %d generating cq notification", sumi_api::rank_);
    incoming_message(sstmac::sumi::transport_message::ptr());
  }
}

dharma::collective_done_message::ptr
sumi_dharma_transport::collective_block(dharma::collective::type_t ty, int tag)
{
  //first we have to loop through the completion queue to see if it already exists
  while(1)
  {
  std::list<message::ptr>::iterator it, end = completion_queue_.start_iteration();
  for (it=completion_queue_.begin(); it != end; ++it){
    message::ptr msg = *it;
    if (msg->class_type() == message::collective_done){
      //this is a collective done message
      collective_done_message::ptr cmsg
        = ptr_safe_cast(collective_done_message, msg);
      if (tag == cmsg->tag() && ty == cmsg->type()){  //done!
        completion_queue_.erase(it);
        completion_queue_.end_iteration();
        return cmsg;
      }
    }
  }

  completion_queue_.end_iteration();
  message::ptr msg = block_until_message();

  if (msg->class_type() == message::collective_done){
    //this is a collective done message
    collective_done_message::ptr cmsg = ptr_safe_cast(collective_done_message, msg);
    if (tag == cmsg->tag() && ty == cmsg->type()){  //done!
      return cmsg;
    }
  }
  completion_queue_.push_back(msg);

  }

}

void
sumi_dharma_transport::do_send_terminate(int dst)
{
  //make a no-op

  //spkt_throw(sprockit::unimplemented_error,
  //  "sumi transport for SST should not send terminates");
}

void
sumi_dharma_transport::schedule_next_heartbeat()
{
  schedule_delay(heartbeat_interval_, new_event(loc_, this, &sumi_dharma_transport::next_heartbeat));
}

void
sumi_dharma_transport::delayed_transport_handle(const dharma::message::ptr &msg)
{
  sstmac::event* done_ev = sstmac::new_event(loc_, this, &transport::handle, msg);
  schedule_delay(sstmac::timestamp(1e-9), done_ev);
}

void
sumi_dharma_transport::schedule_ping_timeout(dharma::pinger* pnger, double to)
{
  sstmac::timestamp next_ping_time = api::now() + to;
  sstmac::event* cb_event = sstmac::new_event(loc_, this, &sumi_dharma_transport::ping_timeout, pnger);
  api::schedule(next_ping_time, cb_event);
}

void
sumi_dharma_transport::ping_timeout(dharma::pinger* pnger)
{
  pnger->execute();
}


}
