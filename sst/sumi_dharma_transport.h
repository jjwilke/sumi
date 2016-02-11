#ifndef sumi_DHARMA_TRANSPORT_H
#define sumi_DHARMA_TRANSPORT_H

#include <sstmac/libraries/sumi/sumi_api.h>
#include <dharma/monitor.h>
#include <dharma/timeout.h>
#include <dharma/message.h>
#include <dharma/collective.h>
#include <dharma/transport.h>
#include <dharma/comm_functions.h>

namespace dharma {

class sumi_dharma_transport :
  public sstmac::sumi::sumi_api,
  public transport
{
 public:
  sumi_dharma_transport();

  virtual void
  init();

  virtual void
  finalize();

  virtual void
  init_factory_params(sprockit::sim_parameters* params);

  void
  finalize_init();

  virtual ~sumi_dharma_transport(){}

  virtual sstmac::sumi::transport_message::payload_ptr
  handle(const sstmac::sumi::transport_message::ptr& msg);

  /**
   * Block on a collective of a particular type and tag
   * until that collective is complete
   * @param ty
   * @param tag
   * @return
   */
  collective_done_message::ptr
  collective_block(dharma::collective::type_t ty, int tag);

  void
  cq_notify();

  double
  wall_time() const;

  message::ptr
  block_until_message();

  message::ptr
  block_until_message(double timeout);

  void
  ping_timeout(pinger* pnger);

 protected:
  void
  do_smsg_send(int dst, const message::ptr &msg);

  void
  do_rdma_put(int dst, const message::ptr& msg);

  void
  do_rdma_get(int src, const message::ptr& msg);

  void
  do_nvram_get(int src, const message::ptr& msg);

  void
  do_send_terminate(int dst);

  void
  do_send_ping_request(int dst);

  void
  delayed_transport_handle(const message::ptr& msg);

  void
  schedule_ping_timeout(pinger* pnger, double to);

  void
  schedule_next_heartbeat();

  void
  go_die();

  void
  go_revive();

};

}

#endif // sumi_DHARMA_TRANSPORT_H
