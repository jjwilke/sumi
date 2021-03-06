#include <sumi/collective.h>
#include <sumi/collective_actor.h>
#include <sumi/dense_rank_map.h>
#include <sumi/message.h>
#include <sumi/transport.h>
#include <sumi/domain.h>
#include <sumi/thread_safe_set.h>
#include <sprockit/sim_parameters.h>
#include <sprockit/stl_string.h>

using namespace sprockit::dbg;

/*
#undef debug_printf
#define debug_printf(flags, ...) \
  if (tag_ == 115 || tag_ == 114) std::cout << sprockit::spkt_printf(__VA_ARGS__) << std::endl
*/

RegisterDebugSlot(sumi_collective_init,
 "print all debug output for collectives performed within the sumi framework");
RegisterDebugSlot(sumi_collective,
 "print all debug output for collectives performed within the sumi framework");
RegisterDebugSlot(sumi_collective_sendrecv,
 "print all debug output for individual send/recv operations done by a sumi collective");
RegisterDebugSlot(sumi_collective_round,
 "print all debug output for configuring/running collectives like allreduce based on round-by-round communication");
RegisterDebugSlot(sumi_vote,
 "print all debug output for fault-tolerant voting collectives within the sumi framework");

ImplementFactory(sumi::dag_collective)

namespace sumi {

#define enumcase(x) case x: return #x

const char*
collective::tostr(type_t ty)
{
  switch (ty)
  {
    enumcase(alltoall);
    enumcase(alltoallv);
    enumcase(allreduce);
    enumcase(allgather);
    enumcase(allgatherv);
    enumcase(scatter);
    enumcase(scatterv);
    enumcase(gather);
    enumcase(gatherv);
    enumcase(reduce);
    enumcase(reduce_scatter);
    enumcase(scan);
    enumcase(barrier);
    enumcase(dynamic_tree_vote);
    enumcase(heartbeat);
    enumcase(bcast);
  }
  spkt_throw_printf(sprockit::value_error,
      "collective::tostr: unknown type %d", ty);
}

void
collective::init(type_t ty, transport *api, domain *dom, int tag, int context)
{
  my_api_ = api;
  dom_ = dom;
  context_ = context;
  complete_ = false;
  tag_ = tag;
  type_ = ty;

  const thread_safe_set<int>& failed = api->failed_ranks(context);
  dense_rank_map rank_map(failed, dom);

  dense_nproc_ = rank_map.dense_rank(dom->nproc());
  dense_me_ = rank_map.dense_rank(dom->my_domain_rank());

  debug_printf(sumi_collective | sumi_vote,
    "Rank %d=%d built collective of size %d in role=%d, tag=%d, context=%d with num_live=%d, failed=%s ",
    my_api_->rank(), dom->my_domain_rank(), dom->nproc(), dense_me_, tag, context, dense_nproc_, failed.to_string().c_str());
}

collective::collective(type_t ty, transport* api, domain* dom, int tag, int context)
{
  init(ty, api, dom, tag, context);
}

void
collective::actor_done(int domain_rank, bool& generate_cq_msg, bool& delete_collective)
{
  generate_cq_msg = false;
  delete_collective = false;

  int& refcount = refcounts_[domain_rank];
  refcount--;
  if (refcount == 0){
    refcounts_.erase(domain_rank);
    generate_cq_msg = true;
  }

  if (refcounts_.empty()){
    delete_collective = true;
  }
}

void
collective::add_actors(collective *coll)
{
  spkt_throw(sprockit::value_error,
    "collective:add_actors: collective of type should not dynamically add actors");
}

void
collective::recv(const collective_work_message_ptr &msg)
{
  switch(msg->payload_type())
  {
    case message::rdma_get_ack:
    case message::rdma_put_ack:
      recv(msg->dense_sender(), msg); //I got an ack because my send data went out
      break;
    default:
      recv(msg->dense_recver(), msg); //all other messages have the correct directionality - I am the recver in the transaction
      break;
  }
}

collective::~collective()
{
}

dag_collective::~dag_collective()
{
  actor_map::iterator it, end = my_actors_.end();
  for (it=my_actors_.begin(); it != end; ++it){
    delete it->second;
  }
}

dag_collective*
dag_collective::construct(const std::string& name, sprockit::sim_parameters *params)
{
  sprockit::sim_parameters* collective_params = params->get_namespace(name);
  return dag_collective_factory::get_param("algorithm", collective_params);
}

dag_collective*
dag_collective::construct(const std::string& name, sprockit::sim_parameters *params, reduce_fxn fxn)
{
  sprockit::sim_parameters* collective_params = params->get_namespace(name);
  dag_collective* coll = dag_collective_factory::get_param("algorithm", collective_params);
  coll->init_reduce(fxn);
  return coll;
}

void
dag_collective::init(type_t type,
  transport *my_api, domain *dom,
  void *dst, void *src,
  int nelems, int type_size,
  int tag,
  bool fault_aware, int context)
{
  collective::init(type, my_api, dom, tag, context);
  fault_aware_ = fault_aware;
  nelems_ = nelems;
  type_size_ = type_size;
  src_buffer_ = src;
  dst_buffer_ = dst;
  dag_collective_actor* actor = new_actor();

  actor->init(type, my_api_, dom_, nelems, type_size, tag_, fault_aware_, context);
  actor->init_buffers(dst_buffer_, src_buffer_);
  actor->init_dag();

  my_actors_[dense_me_] = actor;
  refcounts_[dom_->my_domain_rank()] = my_actors_.size();

  bool has_src = src;
  bool has_dst = dst;
  if ((has_src && !has_dst) || (has_dst && !has_src)){
    spkt_throw(sprockit::illformed_error,
        "collective: must specify both src,dst buffers to be null or non-null, not mixed");
  }
}

void
dag_collective::recv(int target, const collective_work_message::ptr& msg)
{
  debug_printf(sumi_collective | sumi_collective_sendrecv,
    "Rank %d=%d %s got %s:%p from %d=%d on tag=%d for target %d",
    my_api_->rank(), dense_me_,
    collective::tostr(type_),
    message::tostr(msg->payload_type()),
    msg.get(),
    msg->sender(), msg->dense_sender(),
    tag_, target);

  dag_collective_actor* vr = my_actors_[target];
  if (!vr){
    //data-centric collective - this actor does not exist
    pending_.push_back(msg);
    //spkt_throw_printf(sprockit::value_error,
    //  "virtual_bruck_actor::recv: invalid handler %d for endpoint %d for %s on tag=%d ",
    //  target, my_api_->rank(),
    //  msg->to_string().c_str(),
    //  tag_);
  } else {
    vr->recv(msg);
  }
}

void
dag_collective::start()
{
  actor_map::iterator it, end = my_actors_.end();
  for (it = my_actors_.begin(); it != end; ++it){
    dag_collective_actor* actor = it->second;
    actor->start();
  }
}

void
dag_collective::deadlock_check()
{
  std::cout << sprockit::printf("%s collective deadlocked on rank %d, tag %d",
                  tostr(type_), my_api_->rank(), tag_) << std::endl;

  actor_map::iterator it, end = my_actors_.end();
  for (it=my_actors_.begin(); it != end; ++it){
    dag_collective_actor* actor = it->second;
    if (actor)
      actor->deadlock_check();
    else
      spkt_throw_printf(sprockit::null_error,
                        "%s collective deadlocked on rank %d, tag %d, with NULL actor",
                        tostr(type_), my_api_->rank(), tag_);
  }
}

void
dag_collective::add_actors(collective* coll)
{
  dag_collective* ar = static_cast<dag_collective*>(coll);
  { std::map<int, dag_collective_actor*>::iterator it, end = ar->my_actors_.end();
  for (it=ar->my_actors_.begin(); it != end; ++it){
    my_actors_[it->first] = it->second;
  } }

  refcounts_[coll->dom()->my_domain_rank()] = ar->my_actors_.size();

  std::list<collective_work_message::ptr> pending = pending_;
  pending_.clear();
  { std::list<collective_work_message::ptr>::iterator it, end = pending.end();
  for (it=pending.begin(); it != end; ++it){
    collective::recv(*it);
  } }

  ar->my_actors_.clear();
}

}
