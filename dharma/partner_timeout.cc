#include <dharma/partner_timeout.h>

namespace dharma {

void
collective_timeout::time_out(int partner)
{
  actor_->partner_ping_failed(partner);
}

}

