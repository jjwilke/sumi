#include <sumi/monitor.h>
#include <sstmac/hardware/network/network_message.h>

namespace sumi {

class subscription
{
 public:

  std::string
  to_string() const {
    return "subscription";
  }

  int
  cancel(timeout_function* func){
    return functions_.erase(func);
  }

  int
  refcount() const {
    return functions_.refcount();
  }

  void
  append(timeout_function* func){
    functions_.append(func);
  }

  void
  failed(int dst);

 protected:
  function_set functions_;

};

class subscribe_monitor :
  public activity_monitor
{
 public:
  std::string
  to_string() const {
    return "subscribe monitor";
  }

  void
  ping(int dst, timeout_function* func);

  void
  cancel_ping(int dst, timeout_function* func);

  void
  message_received(const message::ptr& msg);

  void
  validate_done();

 protected:
  void
  send_message(int dst, sstmac::hw::network_message::type_t ty);

 protected:
  std::map<int, subscription*> subscriptions_;

};

}
