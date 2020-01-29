#include "nng/nng.h"
#include "nng/protocol/pubsub0/pub.h"
#include "nng/protocol/pubsub0/sub.h"
#include "nng/protocol/reqrep0/req.h"
#include "nng/protocol/reqrep0/rep.h"
#include "nng/protocol/pair0/pair.h"
#include "nng/protocol/pair1/pair.h"

typedef enum XNNG_TYPE 
  { 
   xnng_socket_type,
   xnng_req0_type,
   xnng_rep0_type,
   xnng_pub0_type,
   xnng_sub0_type,
   xnng_pair0_type,
   xnng_pair1_type,
   xnng_aio_type,
   xnng_ctx_type,
   xnng_iov_type,
   xnng_msg_type,
   xnng_url_type,
   xnng_pipe_type,
   xnng_stat_type,
   xnng_dialer_type,
   xnng_stream_type,
   xnng_pipe_ev_type,
   xnng_duration_type,
   xnng_listener_type,
   xnng_sockaddr_type,
   xnng_sockaddr_in_type,
   xnng_sockaddr_zt_type,
   xnng_sockaddr_in6_type,
   xnng_sockaddr_ipc_type,
   xnng_sockaddr_tcp_type,
   xnng_sockaddr_udp_type,
   xnng_sockaddr_path_type,
   xnng_sockaddr_tcp6_type,
   xnng_sockaddr_udp6_type,
   xnng_stream_dialer_type,
   xnng_sockaddr_inproc_type,
   xnng_stream_listener_type} 
  xnng_type;

typedef union XNNG_PTR {
  nng_aio *aio;
  nng_msg *msg;
  nng_stream *stream;
  nng_ctx ctx;
  nng_iov iov;
  nng_url url;
  nng_pipe pipe;
  nng_stat *stat;
  nng_dialer dialer;
  nng_socket socket;
  nng_pipe_ev pipe_ev;
  nng_duration duration;
  nng_listener listener;
  nng_sockaddr sockaddr;
  nng_sockaddr_in sockaddr_in;
  nng_sockaddr_zt sockaddr_zt;
  nng_sockaddr_in6 sockaddr_in6;
  nng_sockaddr_ipc sockaddr_ipc;
  nng_sockaddr_tcp sockaddr_tcp;
  nng_sockaddr_udp sockaddr_udp;
  nng_sockaddr_path sockaddr_path;
  nng_sockaddr_tcp6 sockaddr_tcp6;
  nng_sockaddr_udp6 sockaddr_udp6;
  nng_sockaddr_inproc sockaddr_inproc;
  nng_stream_dialer *stream_dialer;
  nng_stream_listener *stream_listener; } xnng_ptr;

typedef struct KNO_NNG {
  KNO_TAGGED_HEAD;
  xnng_type nng_type;
  xnng_ptr nng_ptr;} *kno_nng;

