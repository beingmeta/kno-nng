#include "nng/nng.h"

typedef enum KNO_NNG_TYPE { 
	kno_nng_aio_type,
	kno_nng_ctx_type,
	kno_nng_iov_type,
	kno_nng_msg_type,
	kno_nng_url_type,
	kno_nng_pipe_type,
	kno_nng_stat_type,
	kno_nng_dialer_type,
	kno_nng_socket_type,
	kno_nng_stream_type,
	kno_nng_pipe_ev_type,
	kno_nng_duration_type,
	kno_nng_listener_type,
	kno_nng_sockaddr_type,
	kno_nng_sockaddr_in_type,
	kno_nng_sockaddr_zt_type,
	kno_nng_sockaddr_in6_type,
	kno_nng_sockaddr_ipc_type,
	kno_nng_sockaddr_tcp_type,
	kno_nng_sockaddr_udp_type,
	kno_nng_sockaddr_path_type,
	kno_nng_sockaddr_tcp6_type,
	kno_nng_sockaddr_udp6_type,
	kno_nng_stream_dialer_type,
	kno_nng_sockaddr_inproc_type,
	kno_nng_stream_listener_type } kno_nng_type;
typedef union KNO_NNG_PTR {
	nng_aio *aio;
	nng_ctx *ctx;
	nng_iov *iov;
	nng_msg *msg;
	nng_url *url;
	nng_pipe *pipe;
	nng_stat *stat;
	nng_dialer *dialer;
	nng_socket *socket;
	nng_stream *stream;
	nng_pipe_ev *pipe_ev;
	nng_duration *duration;
	nng_listener *listener;
	nng_sockaddr *sockaddr;
	nng_sockaddr_in *sockaddr_in;
	nng_sockaddr_zt *sockaddr_zt;
	nng_sockaddr_in6 *sockaddr_in6;
	nng_sockaddr_ipc *sockaddr_ipc;
	nng_sockaddr_tcp *sockaddr_tcp;
	nng_sockaddr_udp *sockaddr_udp;
	nng_sockaddr_path *sockaddr_path;
	nng_sockaddr_tcp6 *sockaddr_tcp6;
	nng_sockaddr_udp6 *sockaddr_udp6;
	nng_stream_dialer *stream_dialer;
	nng_sockaddr_inproc *sockaddr_inproc;
	nng_stream_listener *stream_listener; } kno_nng_ptr;

typedef struct KNO_NNG {
  KNO_TAGGED_HEAD;
  kno_nng_type nng_type;
  kno_nng_ptr nng_ptr;} *kno_nng;

