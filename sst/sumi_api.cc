#include <sst/sumi_api.h>
#include <sst/sumi_transport.h>
#include <sstmac/software/process/app_manager.h>
#include <sstmac/software/process/app.h>
#include <sprockit/util.h>

using namespace sstmac;
using namespace sstmac::sw;

namespace sumi {

SpktRegister("sumi", api, sumi_transport, "Create bindings for SUMI runtime");

static sumi_transport*
current_transport()
{
  thread* t = thread::current();
  return t->get_api<sumi_transport> ();
}

transport*
sumi_api()
{
  return current_transport();
}

void
comm_init()
{
  current_transport()->init();
}

void
comm_kill_process()
{
  current_transport()->kill_process();
}

const thread_safe_set<int>&
comm_failed_ranks()
{
  return current_transport()->failed_ranks();
}

const thread_safe_set<int>&
comm_failed_ranks(int context)
{
  return current_transport()->failed_ranks(context);
}

void
comm_kill_node()
{
  current_transport()->kill_node();
  throw terminate_exception();
}

void
comm_finalize()
{
  current_transport()->finalize();
}

void
comm_vote(int vote, int tag, vote_fxn fxn, int context, domain* dom)
{
  current_transport()->dynamic_tree_vote(vote, tag, fxn, context, dom);
}

void
comm_start_heartbeat(double interval)
{
  current_transport()->start_heartbeat(interval);
}

void
comm_stop_heartbeat()
{
  current_transport()->stop_heartbeat();
}

void
comm_allreduce(void *dst, void *src, int nelems, int type_size, int tag, reduce_fxn fxn, bool fault_aware, int context, domain* dom)
{
  current_transport()->allreduce(dst, src, nelems, type_size, tag, fxn, fault_aware, context, dom);
}

void
comm_allgather(void *dst, void *src, int nelems, int type_size, int tag, bool fault_aware, int context, domain* dom)
{
  current_transport()->allgather(dst, src, nelems, type_size, tag, fault_aware, context, dom);
}

void
comm_bcast(void *buffer, int nelems, int type_size, int tag, bool fault_aware, int context, domain *dom)
{
  current_transport()->bcast(buffer, nelems, type_size, tag, fault_aware, context, dom);
}

void
comm_barrier(int tag, bool fault_aware, domain* dom)
{
  current_transport()->barrier(tag, fault_aware, dom);
}

collective_done_message::ptr
comm_collective_block(collective::type_t ty, int tag)
{
  return current_transport()->collective_block(ty, tag);
}

void
comm_cancel_ping(int dst, timeout_function* func)
{
  current_transport()->cancel_ping(dst, func);
}

void
comm_ping(int dst, timeout_function* func)
{
  current_transport()->ping(dst, func);
}

int comm_rank()
{
  return current_transport()->rank();
}

int comm_nproc()
{
  return current_transport()->nproc();
}

/**
    @param dst The destination to send to
*/
void
comm_send(int dst, message::payload_type_t ty, const message::ptr& msg)
{
  msg->set_class_type(message::pt2pt);
  current_transport()->smsg_send(dst, ty, msg);
}

void
comm_send_header(int dst, const message::ptr& msg)
{
  msg->set_class_type(message::pt2pt);
  current_transport()->send_header(dst, msg);
}

void
comm_send_payload(int dst, const message::ptr& msg)
{
  msg->set_class_type(message::pt2pt);
  current_transport()->send_payload(dst, msg);
}

void
comm_rdma_put(int dst, const message::ptr& msg)
{
  msg->set_class_type(message::pt2pt);
  current_transport()->rdma_put(dst, msg);
}

void
comm_nvram_get(int dst, const message::ptr& msg)
{
  msg->set_class_type(message::pt2pt);
  current_transport()->nvram_get(dst, msg);
}

void
comm_rdma_get(int dst, const message::ptr& msg)
{
  msg->set_class_type(message::pt2pt);
  current_transport()->rdma_get(dst, msg);
}

message::ptr
comm_poll()
{
  return current_transport()->blocking_poll();
}

double
wall_time()
{
  return current_transport()->wall_time();
}

int
comm_partner(long nid)
{
  return current_transport()->get_partner(node_id(nid));
}

void compute(double sec)
{
  thread* thr = thread::current();
  app* my_app = safe_cast(app, thr);
  my_app->compute(timestamp(sec));
}



}

