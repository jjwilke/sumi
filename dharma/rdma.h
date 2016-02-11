#ifndef RDMA_interface_H
#define RDMA_interface_H

#include <dharma/config.h>
#include <sprockit/serializer_fwd.h>

#define ImplementRdmaAPI \
public: \
 dharma::public_buffer& local_buffer() { return local_buffer_; } \
 dharma::public_buffer& remote_buffer() { return remote_buffer_; } \
private: \
 dharma::public_buffer local_buffer_; \
 dharma::public_buffer remote_buffer_;


#endif // RDMA_H
