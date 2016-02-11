#include <cstring>
#include <sumi/transport.h>
#include <sumi/dynamic_tree_vote.h>
#include <sumi/allreduce.h>
#include <sumi/allgather.h>
#include <sumi/domain.h>
#include <sumi/bcast.h>
#include <sprockit/stl_string.h>
#include <sprockit/sim_parameters.h>
#include <sprockit/keyword_registration.h>

RegisterDebugSlot(sumi);
ImplementFactory(sumi::transport);

RegisterKeywords("lazy_watch", "eager_cutoff", "use_put_protocol");

#define START_PT2PT_FUNCTION(dst) \
  start_function(); \
  if (is_failed(dst)) \
    return

#define START_COLLECTIVE_FUNCTION() \
  start_function()

#define END_PT2PT_FUNCTION() \
  end_function()

#define END_COLLECTIVE_FUNCTION() \
  end_function()

namespace sumi {

const int options::initial_context = -2;

transport::transport() :
  inited_(false),
  finalized_(false),
  eager_cutoff_(512),
  lazy_watch_(false),
  heartbeat_active_(false),
  heartbeat_running_(false),
  next_transaction_id_(12),
  max_transaction_id_(0),
  is_dead_(false),
  use_put_protocol_(false),
  use_hardware_ack_(false),
  global_domain_(0),
  nspares_(0),
  recovery_lock_(0)
{
  heartbeat_tag_start_ = 1e9;
  heartbeat_tag_stop_ = heartbeat_tag_start_ + 10000;
  heartbeat_tag_ = heartbeat_tag_start_;
}

void
transport::validate_api()
{
  if (!inited_ || finalized_){
    spkt_throw(sprockit::illformed_error,
    "SUMI transport calling function while not inited or already finalized");
  }
}

void
transport::set_use_hardware_ack(bool flag)
{
  if (flag && !supports_hardware_ack()){
    spkt_throw(sprockit::value_error,
      "transport::chosen transport does not support hardware acks");
  }
  use_hardware_ack_ = flag;
}

void
transport::lock()
{
  lock_.lock();
}

void
transport::unlock()
{
  lock_.unlock();
}

void
transport::die()
{
  is_dead_ = true;
  go_die();
  throw terminate_exception();
}

void
transport::revive()
{
  is_dead_ = false;
  go_revive();
}

domain*
transport::global_dom() const
{
  return global_domain_;
}

void
transport::init()
{
  //THIS SHOULD ONLY BE CALLED AFTER RANK and NPROC are known
  inited_ = true;
  global_domain_ = new global_domain(this);
  const char* nspare_str = getenv("SUMI_NUM_SPARES");
  if (nspare_str){
    int nspares = atoi(nspare_str);
    init_spares(nspares);
  }
}

void
transport::init_spares(int nspares)
{

}

void
transport::finalize()
{
  clean_up();
  //this should really loop through and kill off all the pings
  //so none of them execute
  finalized_ = true;
  debug_printf(sprockit::dbg::sumi,
      "Rank %d sending finalize terminate to %s",
      rank_, failed_ranks_.to_string().c_str());
  thread_safe_set<int>::iterator it, end = failed_ranks_.start_iteration();
  for (it=failed_ranks_.begin(); it != end; ++it){
    int dst = *it;
    debug_printf(sprockit::dbg::sumi,
        "Rank %d sending finalize terminate to %d",
        rank_, dst);
    send_terminate(dst);
  }
  failed_ranks_.end_iteration();
}

void
transport::operation_done(const message::ptr &msg)
{
  completion_queue_.push_back(msg);
  cq_notify();
}

void
transport::renew_pings()
{
  monitor_->renew_pings(wall_time());
}

message::ptr
transport::blocking_poll(message::payload_type_t ty)
{
  std::list<message::ptr>::iterator it, end = completion_queue_.start_iteration();
  for (it=completion_queue_.begin(); it != end; ++it){
    message::ptr msg = *it;
    if (msg->payload_type() == ty){
      completion_queue_.erase(it);
      return msg;
    }
  }
  completion_queue_.end_iteration();

  while (1){
    message::ptr msg = blocking_poll();
    if (msg->payload_type() == ty){
      return msg;
    } else {
      completion_queue_.push_back(msg);
    }
  }
}

message::ptr
transport::blocking_poll()
{
  bool empty;
  message::ptr dmsg = completion_queue_.pop_front_and_return(empty);
  if (empty){
    debug_printf(sprockit::dbg::sumi,
      "Rank %d blocking_poll: cq empty, blocking", rank_);
    return block_until_message();
  }
  else {
    debug_printf(sprockit::dbg::sumi,
      "Rank %d blocking_poll: cq entry, not blocking", rank_);
    return dmsg;
  }
}

message::ptr
transport::blocking_poll(double timeout)
{
  bool empty;
  message::ptr dmsg = completion_queue_.pop_front_and_return(empty);
  if (empty){
    debug_printf(sprockit::dbg::sumi,
      "Rank %d blocking_poll: cq empty, blocking until timeout %8.4e",
      rank_, timeout);
    return block_until_message(timeout);
  } else {
    debug_printf(sprockit::dbg::sumi,
      "Rank %d blocking_poll: cq entry, not blocking", rank_);
    return dmsg;
  }
}


void
transport::send(int dst, const message::ptr &msg)
{
  if (use_eager_protocol(msg->byte_length())){
    send_payload(dst, msg);
  } else {
    send_unexpected_rdma(dst, msg);
  }
}

void
transport::notify_collective_done(const collective_done_message::ptr &msg)
{
  collective* coll = collectives_[msg->type()][msg->tag()];
  if (!coll){
    spkt_throw(sprockit::value_error,
      "transport::notify_collective_done: invalid collective of type %s, tag %d",
       collective::tostr(msg->type()), msg->tag());   
  }
  finish_collective(coll, msg);
}

void
transport::handle_unexpected_rdma_header(const message::ptr& msg)
{
  msg->local_buffer() = allocate_public_buffer(msg->byte_length());
  debug_printf(sprockit::dbg::sumi,
    "Rank %d allocated unexpected buffer %p to recv rdma message from sender %d",
    rank_, msg->local_buffer().ptr, msg->sender());
  rdma_get(msg->sender(), msg);
}

void
transport::clean_up()
{
  std::list<collective*>::iterator it, end = todel_.end();
  for (it=todel_.begin(); it != end; ++it){
    delete *it;
  }
  todel_.clear();
}

void
transport::handle(const message::ptr& msg)
{
  debug_printf(sprockit::dbg::sumi,
    "Rank %d got message of class %s, payload %s for sender %d, recver %d",
     rank_, message::tostr(msg->class_type()), message::tostr(msg->payload_type()),
     msg->sender(), msg->recver());

  //we might have collectives to delete and cleanup
  if (!todel_.empty()){
    clean_up();
  }

  switch (msg->class_type())
  {
  case message::terminate:
  case message::collective_done:
  case message::pt2pt: {
    operation_done(msg);
    break;
  }
  case message::unexpected:
    switch (msg->payload_type()){
      case message::header:
        handle_unexpected_rdma_header(msg);
        break;
      default:
        operation_done(msg);
        break;
    }
    break;
  case message::collective: {
    collective_work_message::ptr cmsg = ptr_safe_cast(collective_work_message, msg);
    int tag = cmsg->tag();
    collective::type_t ty = cmsg->type();
    tag_to_collective_map::iterator it = collectives_[ty].find(tag);
    if (it == collectives_[ty].end()){
      debug_printf(sprockit::dbg::sumi_collective_sendrecv,
        "Rank %d, queuing %p %s %s from %d on tag %d for type %s",
        rank_, msg.get(),
        message::tostr(msg->payload_type()),
        message::tostr(msg->class_type()),
        msg->sender(),
        tag, collective::tostr(ty));
        //message for collective we haven't started yet
        pending_collective_msgs_[ty][tag].push_back(cmsg);
    }
    else {
      collective* coll = it->second;
      coll->recv(cmsg);
    }
    break;
  }
  case message::ping: {  
    monitor_->message_received(msg);
    break;
  }
  case message::no_class: {
      spkt_throw_printf(sprockit::value_error,
        "transport::handle: got message %s with no class of type %s",
        msg->to_string().c_str(),
        message::tostr(msg->payload_type()));
  }
  default: {
      spkt_throw_printf(sprockit::value_error,
        "transport::handle: got unknown message class %d",
        msg->class_type());
  }
  }
}

void
transport::send_self_terminate()
{
  message::ptr msg = new message;
  msg->set_class_type(message::terminate);
  send_header(rank_, msg); //send to self
}

void
transport::send_terminate(int dst)
{
  start_function();
  do_send_terminate(dst);
  end_function();
}

void
transport::cancel_ping(int dst, timeout_function* func)
{
  monitor_->cancel_ping(dst, func);
}

bool
transport::ping(int dst, timeout_function* func)
{ 
  CHECK_IF_I_AM_DEAD(return false);
  validate_api();
  if (is_failed(dst)){
    return true;
  }
  else {
    monitor_->ping(dst, func);
    return false;
  }
}

void
transport::stop_watching(int dst, timeout_function* func)
{
  if (!lazy_watch_){
    cancel_ping(dst, func);
    return;
  }

  watcher_map::iterator it = watchers_.find(dst);
  if (it==watchers_.end()){
    spkt_throw_printf(sprockit::value_error,
      "transport not watching %d, cannot erase", dst);
  }
  function_set& fset = it->second;
  int refcount = fset.erase(func);
  if (refcount == 0){
    watchers_.erase(it);
  }
}

bool
transport::start_watching(int dst, timeout_function *func)
{
  if (!lazy_watch_){
    return ping(dst, func);
  }

  validate_api();
  if (is_failed(dst)){
    return true;
  }
  else {
    debug_printf(sprockit::dbg::sumi | sprockit::dbg::sumi_ping,
      "Rank %d start watching %d", rank_, dst);
    function_set& fset = watchers_[dst];
    fset.append(func);
    return false;
  }
}

void
transport::fail_watcher(int dst)
{
  if (!lazy_watch_)
    return;

  std::map<int, function_set>::iterator it = watchers_.find(dst);
  if (it == watchers_.end())
    return;

  debug_printf(sprockit::dbg::sumi | sprockit::dbg::sumi_ping,
    "Rank %d failing watcher for %d", rank_, dst);
  function_set& fset = it->second;
  fset.timeout_all_listeners(dst);
  watchers_.erase(dst);
}

void
transport::init_factory_params(sprockit::sim_parameters* params)
{
  monitor_ = activity_monitor_factory::get_optional_param("activity_monitor", "ping",
                                        params, this);

  eager_cutoff_ = params->get_optional_int_param("eager_cutoff", 512);
  use_put_protocol_ = params->get_optional_bool_param("use_put_protocol", false);

  lazy_watch_ = params->get_optional_bool_param("lazy_watch", true);

  allgathers_[0] = new bruck_collective;
  allreduces_[0] = new wilke_halving_allreduce;
  bcasts_[0] = new binary_tree_bcast_collective;
}

void
transport::dynamic_tree_vote(int vote, int tag, vote_fxn fxn, int context, domain* dom)
{
  if (dom == 0) dom = global_domain_;
  if (dom->nproc() == 1){
    collective_done_message::ptr dmsg = new collective_done_message(tag, collective::dynamic_tree_vote, dom);
    dmsg->set_domain_rank(0);
    dmsg->set_vote(vote);
    votes_done_[tag] = vote_result(vote, thread_safe_set<int>());
    handle(dmsg);
    return;
  }

  START_COLLECTIVE_FUNCTION();
  dynamic_tree_vote_collective* voter = new dynamic_tree_vote_collective(vote, fxn, tag, this, dom, context);
  start_collective(voter);
  END_COLLECTIVE_FUNCTION();
}

template <class Map, class Val, class Key>
bool
pull_from_map(Val& val, const Map& m, const Key& k)
{
  typedef typename Map::iterator iterator;
  iterator it = m.find(k);
  if (m.find(k) == m.end()){
    return false;
  } else {
    val = it->second;
    m.erase(it);
    return true;
  }
}

template <class Map, class Val, class Key1, class Key2>
bool
pull_from_map(Val& val, const Map& m, const Key1& k1, const Key2& k2)
{
  typedef typename Map::iterator iterator;
  iterator it = m.find(k1);
  if (it == m.end()) return false;

  bool ret = check_map(val, it->second, k2);
  if (it->second.empty()) m.erase(it);
  return ret;
}

template <class Map, class Val, class Key1, class Key2, class Key3>
bool
pull_from_map(Val& val, const Map& m, const Key1& k1, const Key2& k2, const Key3& k3)
{
  typedef typename Map::iterator iterator;
  iterator it = m.find(k1);
  if (it == m.end()) return false;

  bool ret = check_map(val, it->second, k2, k3);
  if (it->second.empty()) m.erase(it);
  return ret;
}


void
transport::deliver_pending(collective* coll, int tag, collective::type_t ty)
{
  std::list<collective_work_message::ptr> pending = pending_collective_msgs_[ty][tag];
  pending_collective_msgs_[ty].erase(tag);
  std::list<collective_work_message::ptr>::iterator it, end = pending.end();

  for (it = pending.begin(); it != end; ++it){
    collective_work_message::ptr msg = *it;
    coll->recv(msg);
  }
}

void
transport::start_heartbeat(double interval)
{
  if (heartbeat_active_){
    spkt_throw_printf(sprockit::illformed_error,
        "sumi_api::start_heartbeat: heartbeat already active");
    return;
  }

  heartbeat_active_ = true;
  heartbeat_interval_ = interval;
  do_heartbeat(options::initial_context);
}

void
transport::stop_heartbeat()
{
  heartbeat_active_ = false;
}

void
transport::vote_done(const collective_done_message::ptr& dmsg)
{
  //if we have some failures, let the watchers know that things have failed
  thread_safe_set<int>::iterator it, end = dmsg->failed_procs().start_iteration();
  for (it=dmsg->failed_procs().begin(); it != end; ++it){
    fail_watcher(*it);
  }
  dmsg->failed_procs().end_iteration();

  if (is_heartbeat(dmsg)){
    dmsg->set_type(collective::heartbeat);
    if (heartbeat_active_){
      schedule_next_heartbeat();
    }
    heartbeat_running_ = false;
    if (dmsg->failed()){
      //only put message in the queue if new failures have occurred
      //we don't need to notify if no new failures
      operation_done(dmsg);
    }
  } else {
    //always pass on a notification if it's a regular voting collective
    operation_done(dmsg);
  }
}

void
transport::next_heartbeat()
{
  //because of weirdness in scheduling
  //we might get a heartbeat request after finalizing
  if (!finalized_)
    do_heartbeat(heartbeat_tag_);
}

void
transport::do_heartbeat(int prev_context)
{
  CHECK_IF_I_AM_DEAD(return);

  heartbeat_running_ = true;
  if (heartbeat_tag_ == heartbeat_tag_stop_){
    heartbeat_tag_ = heartbeat_tag_start_;
  }
  else {
    ++heartbeat_tag_;
  }

  if (nproc() == 1)
    return;

  int vote = 1;
  dynamic_tree_vote_collective* voter = new dynamic_tree_vote_collective(
    vote, &And<int>::op, heartbeat_tag_, this, global_domain_, prev_context);
  collectives_[collective::dynamic_tree_vote][heartbeat_tag_] = voter;
  voter->start();
  deliver_pending(voter, heartbeat_tag_, collective::dynamic_tree_vote);

}

void
transport::validate_collective(collective::type_t ty, int tag)
{
  tag_to_collective_map::iterator it = collectives_[ty].find(tag);
  if (it == collectives_[ty].end()){
    return; // all good
  }

  collective* coll = it->second;
  if (!coll){
   spkt_throw_printf(sprockit::illformed_error,
    "sumi_api::validate_collective: lingering null collective of type %s with tag %d",
    collective::tostr(ty), tag);
  }

  if (coll->persistent() && coll->complete()){
    return; // all good
  }

  spkt_throw_printf(sprockit::illformed_error,
    "sumi_api::validate_collective: cannot overwrite collective of type %s with tag %d",
    collective::tostr(ty), tag);
}

void
transport::start_collective(collective* coll)
{
  int tag = coll->tag();
  collective::type_t ty = coll->type();
  START_COLLECTIVE_FUNCTION();
  if (tag >= heartbeat_tag_start_ && tag <= heartbeat_tag_stop_){
    spkt_throw_printf(sprockit::value_error,
        "sumi::start_collective: %s tag %d is reserved for heartbeats",
        collective::tostr(ty), tag);
  }

  //validate_collective(ty, tag);
  collective*& existing = collectives_[ty][tag];
  if (existing){
    coll->start();
    existing->add_actors(coll);
    delete coll;
  } else {
    existing = coll;
    coll->start();
    deliver_pending(coll, tag, ty);
  }
  END_COLLECTIVE_FUNCTION();
}

dag_collective*
transport::pick_collective(collective::type_t ty, int size, std::map<int,dag_collective*>& coll_map)
{
  std::map<int,dag_collective*>::iterator it, end = coll_map.end();
  for (it=coll_map.begin(); it != end; ++it){
    int nextSize = it->first;
    if (size >= nextSize){
      return it->second->clone();
    }
  }
  spkt_throw_printf(sprockit::value_error,
    "no collective registered for type %s and size %d",
     collective::tostr(ty), size);
  return 0;
}

void
transport::deadlock_check()
{
  collective_map::iterator it, end = collectives_.end();
  for (it=collectives_.begin(); it != end; ++it){
    tag_to_collective_map& next = it->second;
    tag_to_collective_map::iterator cit, cend = next.end();
    for (cit=next.begin(); cit != cend; ++cit){
      collective* coll = cit->second;
      if (!coll->complete()){
        coll->deadlock_check();
      }
    }
  }
}

dag_collective*
transport::build_collective(collective::type_t ty,
  std::map<int,dag_collective*>& algorithms,
  void* dst, void *src,
  int nelems, int type_size,
  int tag,
  bool fault_aware,
  int context, domain* dom,
  reduce_fxn fxn)
{
  CHECK_IF_I_AM_DEAD(return 0);
  if (dom == 0) dom = global_domain_;
  if (dom->nproc() == 1){
    if (dst && src && (dst != src)){
      ::memcpy(dst, src, nelems*type_size);
    }
    collective_done_message::ptr dmsg = new collective_done_message(tag, ty, dom);
    dmsg->set_domain_rank(0);
    dmsg->set_result(dst);
    handle(dmsg);
    return 0; //null indicates no work to do
  }

  dag_collective* coll = pick_collective(ty, type_size*nelems, algorithms);
  coll->init_reduce(fxn); //probably does nothing
  coll->init(ty, this, dom, dst, src, nelems, type_size, tag, fault_aware, context);
  return coll;
}

void
transport::allreduce(void* dst, void *src, int nelems, int type_size, int tag, reduce_fxn fxn, bool fault_aware, int context, domain* dom)
{
  dag_collective* coll = build_collective(collective::allreduce, allreduces_,
    dst, src, nelems, type_size, tag, fault_aware, context, dom, fxn);
  if (coll){
    start_collective(coll);
  }
}

void
transport::bcast(void *buf, int nelems, int type_size, int tag, bool fault_aware, int context, domain* dom)
{
  dag_collective* coll = build_collective(collective::bcast, bcasts_,
    buf, buf, nelems, type_size, tag, fault_aware, context, dom);
  if (coll)
    start_collective(coll);
}

void
transport::allgather(void *dst, void *src, int nelems, int type_size, int tag, bool fault_aware, int context, domain* dom)
{
  dag_collective* coll = build_collective(collective::allgather, allgathers_,
    dst, src, nelems, type_size, tag, fault_aware, context, dom);
  if (coll)
    start_collective(coll);
}

void
transport::finish_collective(collective* coll, const collective_done_message::ptr& dmsg)
{
  bool deliver_cq_msg; bool delete_collective;
  coll->actor_done(dmsg->domain_rank(), deliver_cq_msg, delete_collective);
  debug_printf(sprockit::dbg::sumi,
    "Rank %d finishing collective of type %s tag %d and failures %s",
    rank_, collective::tostr(dmsg->type()), dmsg->tag(),
    dmsg->failed_procs().to_string().c_str());

  if (!deliver_cq_msg)
    return;

  coll->complete();
  collective::type_t ty = dmsg->type();
  int tag = dmsg->tag();
  if (delete_collective && !coll->persistent()){ //otherwise collective must exist FOREVER
    collectives_[ty].erase(tag);
    todel_.push_back(coll);
  }

  if (ty == collective::dynamic_tree_vote){
    //this requires some extra processing
    //and doesn't always generate an operation done
    votes_done_[tag] = vote_result(dmsg->vote(), dmsg->failed_procs());
    vote_result& prev_context = votes_done_[coll->context()];
    //merge the previous context
    votes_done_[tag].failed_ranks.insert_all(prev_context.failed_ranks);
    failed_ranks_.insert_all(votes_done_[tag].failed_ranks);
    vote_done(dmsg);
  } else {
    //this always generates an operation done
    operation_done(dmsg);
  }
  pending_collective_msgs_[ty].erase(tag);
  debug_printf(sprockit::dbg::sumi,
    "Rank %d finished collective of type %s tag %d -> known failures are %s",
    rank_, collective::tostr(dmsg->type()), dmsg->tag(), 
    failed_ranks_.to_string().c_str());
}

void
transport::barrier(int tag, bool fault_aware, domain* dom)
{
  //use a zero-size allgather to execute the barrier
  dag_collective* coll = build_collective(collective::barrier, allgathers_,
    0, 0, 0, 0, tag, fault_aware, options::initial_context, dom);
  if (coll) start_collective(coll);
}

static const thread_safe_set<int> empty_set;

const thread_safe_set<int>&
transport::failed_ranks(int context) const
{
  if (context == options::initial_context){
    return empty_set;
  }

  vote_map::const_iterator it = votes_done_.find(context);
  if (it == votes_done_.end()){
    spkt_throw_printf(sprockit::value_error,
        "sumi_api::failed_rank: unknown or uncommitted context %d on rank %d",
        context, rank_);
  }
  return it->second.failed_ranks;
}

void
transport::send_ping_request(int dst)
{
  START_PT2PT_FUNCTION(dst);
  do_send_ping_request(dst);
  END_PT2PT_FUNCTION();
}

static const uint32_t recovery_bitmask = 1<<31;

void
transport::start_recovery()
{
  //set the recovery bit
  recovery_lock_ |= recovery_bitmask;
  uint32_t num_ops_pending = recovery_lock_ ^ recovery_bitmask;
  while (num_ops_pending > 0){
    //allow all operations to complete
    //recovery lock ensures no new operations will run
    uint32_t check_tag = recovery_lock_;
    num_ops_pending = check_tag ^ recovery_bitmask;
  }
}

void
transport::start_function()
{
  const int recovery_bitmask = 1;
  uint32_t check_tag = recovery_lock_++;
  uint32_t recovery_bit = check_tag & recovery_bitmask;
  while (recovery_bit > 0){
    //if the recovery bit is set, we are recovering from a failure
    check_tag = (uint32_t) recovery_lock_;
    recovery_bit = check_tag & recovery_bitmask;
  }
}

void
transport::end_function()
{
  recovery_lock_--;
}

void
transport::smsg_send(int dst, message::payload_type_t ev, const message::ptr& msg, bool needs_ack)
{
  CHECK_IF_I_AM_DEAD(return);
  START_PT2PT_FUNCTION(dst);

  configure_send(dst, ev, msg);
  msg->set_needs_send_ack(needs_ack);


  debug_printf(sprockit::dbg::sumi,
    "Rank %d SUMI sending short message to %d",
    rank_, dst);

  if (dst == rank_) {
    //deliver to self
    delayed_transport_handle(msg);
  }
  else {
    do_smsg_send(dst, msg);
  }

  END_PT2PT_FUNCTION();
}

void
transport::rdma_get(int src, const message::ptr &msg, bool needs_send_ack, bool needs_recv_ack)
{

  CHECK_IF_I_AM_DEAD(return);
  START_PT2PT_FUNCTION(src);

  configure_send(src, message::rdma_get, msg);
  msg->set_needs_send_ack(needs_send_ack);
  msg->set_needs_recv_ack(needs_recv_ack);
  do_rdma_get(src, msg);

  END_PT2PT_FUNCTION();
}

void
transport::rdma_get(int src, const message::ptr &msg)
{
  bool needs_send_ack = msg->class_type() != sumi::message::ping;
  rdma_get(src, msg, needs_send_ack, true /*ack recv*/);
}

void
transport::start_transaction(const message::ptr &msg)
{
  CHECK_IF_I_AM_DEAD(return);
  lock();
  int tid = next_transaction_id_++;
  next_transaction_id_ = next_transaction_id_ % max_transaction_id_;
  message::ptr& entry = transactions_[tid];
  if (entry){
    spkt_throw(sprockit::value_error,
      "too many transactions started simultaneously");
  }
  msg->set_transaction_id(tid);
  entry = msg;
  unlock();
}

message::ptr
transport::finish_transaction(int tid)
{
  lock();
  std::map<int,message::ptr>::iterator it = transactions_.find(tid);
  if (it == transactions_.end()){
     spkt_throw_printf(sprockit::value_error,
      "invalid transaction id %d", tid);
  }
  message::ptr msg = it->second;
  transactions_.erase(it);
  unlock();
  return msg;
}

void
transport::rdma_put(int dst, const message::ptr &msg, bool needs_send_ack, bool needs_recv_ack)
{
  CHECK_IF_I_AM_DEAD(return);
  START_PT2PT_FUNCTION(dst);
  configure_send(dst, message::rdma_put, msg);
  msg->set_needs_send_ack(needs_send_ack);
  msg->set_needs_recv_ack(needs_recv_ack);
  do_rdma_put(dst, msg);
  END_PT2PT_FUNCTION();
}

void
transport::rdma_put(int dst, const message::ptr &msg)
{
  bool needs_send_ack = msg->transaction_id() >= 0;
  rdma_put(dst, msg, needs_send_ack, true /*ack recv*/);
}

void
transport::nvram_get(int src, const message::ptr &msg)
{
  CHECK_IF_I_AM_DEAD(return);
  START_PT2PT_FUNCTION(src);
  configure_send(src, message::nvram_get, msg);
  do_nvram_get(src, msg);
  END_PT2PT_FUNCTION();
}

void
transport::configure_send(int dst, message::payload_type_t ev, const message::ptr& msg)
{
  switch(ev)
  {
  //this is a bit weird here
  //we want to maintain notation of send/recv dst/src as in MPI
  //for an RDMA get the recver sends a request - thus it is sort of a sender
  //however we want to maintain the notion that the source of the message is where the data lives
  //thus the destination sends a request to the source
  case message::rdma_get:
    msg->set_sender(dst);
    msg->set_recver(rank_);
    break;
  default:
    msg->set_sender(rank_);
    msg->set_recver(dst);
    break;
  }
  msg->set_payload_type(ev);

  if (msg->class_type() == message::no_class){
    spkt_throw_printf(sprockit::value_error,
        "sending message %s with no class",
        msg->to_string().c_str());
  }
}

void
transport::send_header(int dst, const message::ptr& msg)
{
  smsg_send(dst, message::header, msg);
}

void
transport::send_payload(int dst, const message::ptr& msg)
{
  smsg_send(dst, message::eager_payload, msg);
}

void
transport::send_unexpected_rdma(int dst, const message::ptr& msg)
{
  msg->set_class_type(message::unexpected);
  send_rdma_header(dst, msg);
}

void
transport::send_rdma_header(int dst, const message::ptr &msg)
{
  if (use_hardware_ack_){
    start_transaction(msg);
  } 
  send_header(dst, msg);
}

}
