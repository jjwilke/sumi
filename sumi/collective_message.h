#ifndef sumi_api_COLLECTIVE_MESSAGE_H
#define sumi_api_COLLECTIVE_MESSAGE_H

#include <sumi/message.h>
#include <sumi/collective.h>
#include <sumi/thread_safe_set.h>

namespace sumi {

/**
 * @class collective_done_message
 * The message that is actually delivered when calling #sumi::comm_poll
 * This encapsulates all the information about a collective that has completed in the background
 */
class collective_done_message :
  public message
{
 public:
  typedef sprockit::refcount_ptr<collective_done_message> ptr;

 public:
  std::string
  to_string() const {
    return "collective done message";
  }

  collective_done_message(int tag, collective::type_t ty, domain* dom) :
    tag_(tag), result_(0), vote_(0), type_(ty),
    all_ranks_know_failure_(false), dom_(dom)
  {
    class_ = collective_done;
    payload_type_ = none;
  }

  int
  tag() const {
    return tag_;
  }

  collective::type_t
  type() const {
    return type_;
  }

  domain*
  dom() const {
    return dom_;
  }

  void
  set_type(collective::type_t ty) {
    type_ = ty;
  }

  bool
  failed() const {
    return !failed_procs_.empty();
  }

  bool
  succeeded() const {
    return failed_procs_.empty();
  }

  void
  append_failed(int proc) {
    failed_procs_.insert(proc);
  }

  void
  append_failed(const std::set<int>& procs){
    failed_procs_.insert(procs.begin(), procs.end());
  }

  const thread_safe_set<int>&
  failed_procs() const {
    return failed_procs_;
  }

  bool
  all_ranks_know_failure() const {
    return all_ranks_know_failure_;
  }

  void
  set_all_ranks_know_failure(bool flag) {
    all_ranks_know_failure_ = true;
  }

  void
  set_result(void* buf){
    result_ = buf;
  }

  void*
  result() {
    return result_;
  }

  void
  set_vote(int v){
    vote_ = v;
  }

  int
  vote() const {
    return vote_;
  }

  parent_message*
  clone() const;

  int domain_rank() const {
    return domain_rank_;
  }

  void
  set_domain_rank(int rank){
    domain_rank_ = rank;
  }

 protected:
  int tag_;
  void* result_;
  int vote_;
  collective::type_t type_;
  thread_safe_set<int> failed_procs_;
  bool all_ranks_know_failure_;
  int domain_rank_;
  domain* dom_;

};

/**
 * @class collective_work_message
 * Main message type used by collectives
 */
class collective_work_message :
  public message
{

 public:
  typedef sprockit::refcount_ptr<collective_work_message> ptr;

  typedef enum {
    get_data, //recver gets data
    put_data, //sender puts data
    rdma_get_header, //sender sends a header to recver to configure RDMA get
    rdma_put_header, //recver sends a header to sender to configure RDMA put
    eager_payload, //for small messages, no recv header - just send payload
    nack_get_ack,
    nack_put_payload,
    nack_eager,
    nack_get_header, //collective has failed, send fake message nack instead of real one
    nack_put_header //collective has failed, send fake message nack instead of real one
  } action_t;


 public:
  virtual std::string
  to_string() const;

  static const char*
  tostr(action_t action);

  virtual void
  serialize_order(sprockit::serializer& ser);

  action_t
  action() const {
    return action_;
  }

  void
  set_action(action_t a) {
    action_ = a;
  }

  int
  nelems() const {
    return nelems_;
  }

  int
  tag() const {
    return tag_;
  }

  int
  round() const {
    return round_;
  }

  int
  dense_sender() const {
    return dense_sender_;
  }

  int
  dense_recver() const {
    return dense_recver_;
  }

  void
  reverse();

  collective::type_t
  type() const {
    return type_;
  }

  bool
  is_failure_notice() const {
    return !failed_procs_.empty();
  }

  void
  append_failed(int proc) {
    failed_procs_.insert(proc);
  }

  void
  append_failed(const thread_safe_set<int>& failed);

  const std::set<int>&
  failed_procs() const {
    return failed_procs_;
  }

 protected:
  void
  clone_into(collective_work_message* cln) const;

  collective_work_message(
    collective::type_t type,
    action_t action,
    int nelems,
    int type_size,
    int tag, int round,
    int src, int dst) :
    nelems_(nelems),
    tag_(tag),
    type_(type),
    round_(round),
    dense_sender_(src),
    dense_recver_(dst),
    action_(action)
  {
    class_ = collective;
    num_bytes_ = type_size * nelems_;
  }

  collective_work_message(){} //for serialization

 protected:
  int nelems_;

  int tag_;

  collective::type_t type_;

  int round_;

  int dense_sender_;

  int dense_recver_;

  action_t action_;

  std::set<int> failed_procs_;

};

class collective_eager_message :
  public collective_work_message,
  public sprockit::serializable_type<collective_eager_message>
{
  ImplementSerializable(collective_eager_message)
 public:
  typedef sprockit::refcount_ptr<collective_eager_message> ptr;

  collective_eager_message(
    collective::type_t type,
    action_t action,
    void* buffer,
    int nelems,
    int type_size,
    int tag, int round,
    int src, int dst) :
    collective_work_message(type,action,nelems,type_size,tag,round,src,dst),
    buffer_(buffer)
 {
 }

  void*&
  eager_buffer() {
    return buffer_;
  }

  parent_message*
  clone() const {
    collective_eager_message* cln = new collective_eager_message;
    clone_into(cln);
    return cln;
  }

  void
  clone_into(collective_eager_message* cln) const {
    cln->buffer_ = buffer_;
    collective_work_message::clone_into(cln);
  }

  virtual void
  serialize_order(sprockit::serializer& ser);

 protected:
  void* buffer_;
};

class collective_rdma_message :
  public collective_work_message,
  public sprockit::serializable_type<collective_rdma_message>
{
  ImplementSerializable(collective_rdma_message)
  ImplementRdmaAPI
 public:
  collective_rdma_message(
    collective::type_t type,
    action_t action,
    int nelems,
    int type_size,
    int tag, int round,
    int src, int dst) :
    collective_work_message(type,action,nelems,type_size,tag,round,src,dst)
  {
  }

  parent_message*
  clone() const {
    collective_rdma_message* cln = new collective_rdma_message;
    clone_into(cln);
    return cln;
  }

  void
  clone_into(collective_rdma_message* cln) const {
    cln->local_buffer_ = local_buffer_;
    cln->remote_buffer_ = remote_buffer_;
    collective_work_message::clone_into(cln);
  }

  virtual void
  serialize_order(sprockit::serializer& ser);

};

}


#endif // COLLECTIVE_MESSAGE_H
