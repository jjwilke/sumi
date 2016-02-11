#include <dharma/subscription.h>
#include <dharma/transport.h>
#include <sstmac/libraries/sumi/sumi_api.h>
#include <sstmac/hardware/network/network_message.h>
#include <sprockit/util.h>

using namespace sstmac::hw;

namespace dharma {

SpktRegister("subscribe", activity_monitor, subscribe_monitor);

void
subscription::failed(int dst)
{
  while (!functions_.empty()){
    functions_.timeout_all_listeners(dst);
  }
}

void
subscribe_monitor::cancel_ping(int dst, dharma::timeout_function* func)
{
  subscription* sub = subscriptions_[dst];
  if (!sub){
    spkt_throw(sprockit::illformed_error,
        "rank %d has no subscription to monitor rank %d",
        api_->rank(), dst);
  }

  int refcount = sub->cancel(func);
  if (refcount == 0){
    debug_printf(sprockit::dbg::dharma_ping,
        "Rank %d totally canceling ping to partner %d for function %p",
        api_->rank(), dst, func);
    send_message(dst, network_message::monitor_cancel);
    subscriptions_.erase(dst);
  }
  else {
    debug_printf(sprockit::dbg::dharma_ping,
        "Rank %d dropping to refcount=%d for subscription to partner %d for function %p",
        api_->rank(), sub->refcount(), dst, func);
  }
}

void
subscribe_monitor::ping(int dst, timeout_function* func)
{
  subscription*& sub = subscriptions_[dst];
  if (!sub){
    debug_printf(sprockit::dbg::dharma_ping,
        "Rank %d totally new subscription to partner %d for function %p",
        api_->rank(), dst, func);
    sub = new subscription;
    sub->append(func);
    send_message(dst, network_message::monitor_subscribe);
  }
  else {
    debug_printf(sprockit::dbg::dharma_ping,
        "Rank %d attaching to existing subscription to partner %d for function %p refcount=%d",
        api_->rank(), dst, func, sub->refcount());
    sub->append(func);
  }
}

void
subscribe_monitor::send_message(int dst, network_message::type_t ty)
{
  debug_printf(sprockit::dbg::dharma_ping,
    "Rank %d sending subscription message of type %s to rank %d",
    api_->rank(), sstmac::hw::network_message::tostr(ty), dst);
  message::ptr msg = new message(8);
  msg->set_class_type(message::ping);
  sstmac::sumi::sumi_api* my_api = safe_cast(sstmac::sumi::sumi_api, api_);
  my_api->transport_send(msg->byte_length(), msg, ty, api_->rank(), dst, false /*no ack*/);
}

void
subscribe_monitor::message_received(const message::ptr& msg)
{
  //this is like an RDMA get - I am actually the receiver
  int partner = msg->recver();
  subscription* sub = subscriptions_[partner];
  api_->declare_failed(partner);
  if (sub){
    debug_printf(sprockit::dbg::dharma_ping,
      "Rank %d received subscription message from rank %d - timing out %d listeners",
      api_->rank(), partner, sub->refcount());

    sub->failed(partner);
  }
  else {
    //ignoring already known failure - this can happen if we cancel the ping
    debug_printf(sprockit::dbg::dharma_ping,
      "Rank %d ignoring subscription message from rank %d",
      api_->rank(), partner);
    subscriptions_.erase(partner);
  }

}

void
subscribe_monitor::validate_done()
{
  debug_printf(sprockit::dbg::dharma_ping,
    "Rank %d validating subscriptions are done: %d subscriptions pending",
    api_->rank(), subscriptions_.size());

  if (!subscriptions_.empty()){
    spkt_throw_printf(sprockit::illformed_error,
        "rank %d still has %d subscriptions pending at finalize",
        api_->rank(), subscriptions_.size());
  }
}

}
