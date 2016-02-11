
#include <pthread.h>
#include <sumi/transport.h>
#include <sumi/domain.h>
#include <sprockit/sim_parameters.h>
#include <sprockit/serializer.h>
#include <sprockit/util.h>
#include <sprockit/stl_string.h>
#include <unistd.h>

#define DEBUG 0

using namespace sumi;

typedef enum {
  allgather,
  allreduce
} type_t;

static const int nreplica = 15;

void
run_test(const std::string& test, transport* t, domain* dom, int nelems, int context, int& tag)
{
  if (t->rank() >= dom->nproc()){
    ++tag; //gotta increment this thought to stay consistent
    return; //I have no part in this
  }

  int nproc = dom->nproc();


  double t_start;
  int *src_buf = 0, *dst_buf = 0, *reduce_buf = 0;
  if (test == "allgather"){
    src_buf = (int*) ::malloc(sizeof(int)*nelems);
    dst_buf = (int*) ::malloc(sizeof(int)*nelems*nproc);
    ::memset(src_buf, 0, nelems*sizeof(int));
    ::memset(dst_buf, 0, nelems*nproc*sizeof(int));
    t_start = t->wall_time();
    t->allgather(dst_buf, src_buf, nelems, sizeof(int), tag, true, context, dom);
  } else if (test == "vote"){
    t_start = t->wall_time();
    t->vote<AndOp>(1, tag, context, dom);
  } else if (test == "allreduce"){
    reduce_buf = (int*) ::malloc(sizeof(int)*nelems);
    ::memset(reduce_buf, 0, nelems*sizeof(int));
    t_start = t->wall_time();
    t->allreduce<int,Add>(reduce_buf, reduce_buf, nelems, tag, true, context, dom);
  }
  message::ptr msg = t->blocking_poll();
  double t_stop = t->wall_time();
  double t_total = t_stop - t_start;
  if (src_buf) ::free(src_buf);
  if (dst_buf) ::free(dst_buf);
  if (reduce_buf) ::free(reduce_buf);

  if (dom->my_domain_rank() == 0){
    printf("Test %s: nelems=%d nproc=%d t=%20.12f ms\n",
      test.c_str(), nelems, nproc, t_total*1e3);
  }

  ++tag;
}

void
run_test(transport* t, domain* dom, int& tag, int* nelems, int ntests, const char* name)
{
  for (int i=0; i < ntests; ++i){
    for (int r=0; r < nreplica; ++r){
      run_test(name, t, dom, nelems[i], options::initial_context, tag);
    }
  }
}

void
run_test(transport* t, domain* dom, int& tag)
{
  int reduce_nelems[] = { 64, 256, 1024, 4096, 16384 };
  int allgather_nelems[] = { 32, 64, 128, 512, 1024 };
  int vote_nelems[] = {1,1,1,1,1};
  int ntests = sizeof(reduce_nelems) / sizeof(int);
 
  run_test(t, dom, tag, reduce_nelems, ntests, "allreduce");
  run_test(t, dom, tag, allgather_nelems, ntests, "allgather");
  run_test(t, dom, tag, vote_nelems, ntests, "vote");

}

void
run_test(transport* t, int& tag)
{
  int me = t->rank();
  int nproc = t->nproc();
  int domain_nproc = nproc;

  while (domain_nproc >= 4){
    domain* dom = new subrange_domain(me, 0, domain_nproc);
    run_test(t,dom,tag);
    domain_nproc /= 2;
  }
}

void
run_vote_test(const char* name, transport* t, domain* dom, int num_failures, int& tag)
{
  if (dom->nproc() <= t->rank()){
    return;
  }

  double t_start = t->wall_time();
  t->vote<AndOp>(1, tag, options::initial_context, dom);
  collective_done_message::ptr dmsg = ptr_safe_cast(collective_done_message, t->blocking_poll());
  double t_stop = t->wall_time();
  double t_total = t_stop - t_start;
  if (t->rank() == 0){
    printf("Test %s: nfailures=%d nproc=%d t=%20.12f ms for failed=%s\n",
      name, num_failures, dom->nproc(), t_total*1e3,
      dmsg->failed_procs().to_string().c_str());
  }

}

void
run_vote_test(const char* name, transport *t, int num_failures, int &tag)
{
  int nproc = t->nproc();
  while (nproc >= 4){
    domain* dom = new subrange_domain(t->rank(), 0, nproc);
    nproc /= 2;
    run_vote_test(name, t, dom, num_failures, tag);
    ++tag;
  }
}

void
run_vote_test(const char* name, transport* t, int max_failures, int* failures, int& tag)
{
  for (int i=0; i < max_failures; ++i){
    int to_fail = failures[i];
    if (t->rank() == to_fail){
      printf("Node %d going down\n", to_fail);
      t->die();
      t->blocking_poll(); //block until I get a terminate message
      t->revive();
      break;
    } else {
      int num_failures = i + 1;
      for (int r=0; r < nreplica; ++r){
        run_vote_test(name, t, num_failures, tag);
      }
    }
  }
  t->clear_failures();
  if (t->rank() == 0){
    for (int i=0 ; i < max_failures; ++i){
      t->send_terminate(failures[i]);
    }
  }

  //make sure that the dead guys wake up before hitting the barrier
  sleep(1);

  /** Make sure everyone got here */
  t->barrier(123456789, false);
  t->blocking_poll();
}

void
run_test()
{
  sprockit::sim_parameters params;
  params["transport"] = DEFAULT_TRANSPORT;
  params["ping_timeout"] = "100ms";
  params["eager_cutoff"] = "512";

  params["use_put_protocol"] = "false";
  params["lazy_watch"] = "true";
  transport* t = transport_factory::get_param("transport", &params);

  t->init();

  int tag = 0;

  if (t->rank() == 0) printf("Eager=512 Lazy=True Protocol=Get Ack=Software\n");
  t->set_use_hardware_ack(false);
  t->set_put_protocol(false);
  run_test(t, tag);

  if (t->rank() == 0) printf("Eager=512 Lazy=True Protocol=Put Ack=Software\n");
  t->set_use_hardware_ack(false);
  t->set_put_protocol(true);
  run_test(t, tag);

  if (t->supports_hardware_ack()){
    if (t->rank() == 0) printf("Eager=512 Lazy=True Protocol=Get Ack=Hardware\n");
    t->set_use_hardware_ack(true);
    t->set_put_protocol(false);
    run_test(t, tag);

    if (t->rank() == 0) printf("Eager=512 Lazy=True Protocol=Put Ack=Hardware\n");
    t->set_use_hardware_ack(true);
    t->set_put_protocol(true);
    run_test(t, tag);
  }



  //now let's do some tests with dead procs
  int max_num_failed_procs = 0;
  int nproc = t->nproc();
  while (nproc >= 4){
    ++max_num_failed_procs;
    nproc /= 4;
  }

  int random_failed_procs[] = {
    2, //4 procs
    11, //16 procs
    36, //64 procs
    106, //256 procs
    835, //1024 procs
    2347 //4096 procs
  };
  run_vote_test("random", t, max_num_failed_procs, random_failed_procs, tag);

  tag = 123456790;
  int seq_failed_procs[] = {
    1, 3, 7, 15, 31, 63, 127
  };
  run_vote_test("sequential", t, max_num_failed_procs, seq_failed_procs, tag);

  t->finalize();
}

int main(int argc, char** argv)
{
  try {
#if DEBUG
  sprockit::debug::turn_on(DEFAULT_TRANSPORT);
  sprockit::debug::turn_on("sumi");
  sprockit::debug::turn_on("sumi_collective");
#endif

    run_test();
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    abort();
  }

  return 0;
}
