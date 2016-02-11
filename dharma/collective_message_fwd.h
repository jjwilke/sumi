#ifndef dharma_api_COLLECTIVE_MESSAGE_FWD_H
#define dharma_api_COLLECTIVE_MESSAGE_FWD_H

#include <sprockit/ptr_type.h>

namespace dharma {

class collective_work_message;
typedef sprockit::refcount_ptr<collective_work_message> collective_work_message_ptr;

class collective_done_message;
typedef sprockit::refcount_ptr<collective_done_message> collective_done_message_ptr;
}

#endif // COLLECTIVE_MESSAGE_FWD_H
