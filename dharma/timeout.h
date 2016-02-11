#ifndef dharma_api_TIMEOUT_H
#define dharma_api_TIMEOUT_H

#include <dharma/message.h>

namespace dharma {

/**
 * @class timeout_function
 * Abstract class for time-outs invoked by #pinger
 * Timeout action can be anything
 */
class timeout_function
{
 public:
  virtual std::string
  to_string() const {
    return "timeout function";
  }

  virtual void
  time_out(int partner) = 0;

  virtual ~timeout_function(){}

};

}

#endif // TIMEOUT_H
