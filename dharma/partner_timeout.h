#ifndef dharma_api_PARTNER_TIMEOUT_H
#define dharma_api_PARTNER_TIMEOUT_H

#include <dharma/collective_actor.h>

namespace dharma
{

/**
 * @class collective_timeout
 * Specific instance of #timeout_function.
 * This informs a #collective that a particular partner
 * has timed out and is considered dead
 */
class collective_timeout :
  public timeout_function
{
 public:
  std::string
  to_string() const {
    return sprockit::printf("collective timeout on tag %d", actor_->tag());
  }

  collective_timeout(collective_actor* actor) :
    actor_(actor)
  {
  }

  void
  time_out(int partner);

 protected:
  /**
   * This is a little bit dangerous.
   * For reference counting reasons,
   * this has to be a simple pointer.
   */
  collective_actor* actor_;

};

}

#endif // PARTNER_TIMEOUT_H
