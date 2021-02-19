/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* nng.c
   This implements Kno bindings to nng
   Copyright (C) 2007-2020 beingmeta, inc.
*/

#ifndef _FILEINFO
#define _FILEINFO __FILE__
#endif

static int knonng_loglevel;
#define U8_LOGLEVEL knonng_loglevel

#define U8_INLINE_IO 1

#include "kno/knosource.h"
#include "kno/lisp.h"
#include "kno/compounds.h"
#include "kno/numbers.h"
#include "kno/eval.h"
#include "kno/xtypes.h"
#include "kno/sequences.h"
#include "kno/texttools.h"
#include "kno/bigints.h"
#include "kno/cprims.h"
#include "kno/knoregex.h"
#include "nng.h"

#include <libu8/libu8.h>
#include <libu8/u8printf.h>
#include <libu8/u8crypto.h>
#include <libu8/u8pathfns.h>

#include <math.h>

static lispval nng_typemap;

static lispval nng_aio_symbol, nng_ctx_symbol, nng_iov_symbol, nng_msg_symbol,
  nng_url_symbol, nng_pipe_symbol, nng_stat_symbol, nng_dialer_symbol,
  nng_socket_symbol, nng_stream_symbol, nng_pipe_ev_symbol, nng_handler_symbol,
  nng_duration_symbol, nng_listener_symbol, nng_sockaddr_symbol,
  nng_sockaddr_in_symbol, nng_sockaddr_zt_symbol, nng_sockaddr_in6_symbol,
  nng_sockaddr_ipc_symbol, nng_sockaddr_tcp_symbol, nng_sockaddr_udp_symbol,
  nng_sockaddr_path_symbol, nng_sockaddr_tcp6_symbol, nng_sockaddr_udp6_symbol,
  nng_stream_dialer_symbol, nng_sockaddr_inproc_symbol,
  nng_stream_listener_symbol, nng_pub0_symbol, nng_sub0_symbol,
  nng_req0_symbol, nng_rep0_symbol;

static lispval get_typesym(xnng_type type)
{
  lispval key = KNO_INT(((int)type));
  return kno_hashtable_get((kno_hashtable)nng_typemap,key,KNO_FALSE);
}

static kno_lisp_type kno_nng_type;
#define KNO_NNG_TYPE 0xA039F80

struct KNO_NNG *kno_nng_create(xnng_type type)
{
  struct KNO_NNG *fresh = u8_alloc(struct KNO_NNG);
  KNO_INIT_FRESH_CONS(fresh,kno_nng_type);
  fresh->typetag = get_typesym(type);
  fresh->typeinfo = NULL;
  fresh->nng_type = type;
  fresh->nng_deps = KNO_EMPTY;
  return fresh;
}

static int unparse_nng_wrapper(u8_output out,lispval x)
{
  struct KNO_NNG *nng = (kno_nng) x;
  u8_printf(out,"#<NNG %q #!0x%lx>",nng->typetag,
	    ((unsigned long long)x));
  return 1;
}

DEFC_PRIM("nng?",nngp_prim,
	     KNO_MAX_ARGS(2)|KNO_MIN_ARGS(1),
	     "returns #t if *obj* is an NNG object and has the "
	     "typetag *tag* (if provided)",
	     {"x",kno_any_type,KNO_VOID},
	     {"tag",kno_symbol_type,KNO_VOID})
static lispval nngp_prim(lispval x,lispval tag)
{
  if (!(KNO_TYPEP(x,kno_nng_type)))
    return KNO_FALSE;
  else if ( (KNO_VOIDP(tag)) || (KNO_FALSEP(tag)) )
    return KNO_TRUE;
  else {
    kno_nng wrapper = (kno_nng) x;
    if (wrapper->typetag == tag)
      return KNO_TRUE;
    else return KNO_FALSE;}
}

static lispval pubsock_symbol;

static lispval mkerr(int rv,u8_context cxt,lispval obj,u8_string details)
{
  return kno_err(nng_strerror(rv),cxt,details,obj);
}

#define CLOSE_CASE(typename) \
  case xnng_ ## typename ## _type: \
  rv = nng_ ## typename ## _close(ref->nng_ptr.typename); break;

#define CHECK_NNG_TYPE(x,caller)					\
  if (!(KNO_TYPEP(x,kno_nng_type))) return kno_err("NotNNG",caller,NULL,x);

#define CHECK_NNG_SUBTYPE(x,subtype,caller)				\
  if (!(KNO_TYPEP(x,kno_nng_type))) return kno_err("NotNNG",caller,NULL,x); \
  else if ( (((kno_nng)(x))->nng_type) != (subtype) )			\
    return kno_err("TypeError(NNG)",caller # subtype,NULL,x);	\
  else NO_ELSE;

#define NNG_GET(x,field) (((kno_nng)x)->nng_ptr.field)

static int socketp(kno_nng ptr)
{
  switch (ptr->nng_type) {
  case xnng_socket_type: case xnng_req0_type: case xnng_rep0_type:
  case xnng_pub0_type: case xnng_sub0_type:
    return 1;
  default:
    return 0;}
}

#define CHECK_SOCKETP(arg,caller)						\
  if (!(KNO_TYPEP(arg,kno_nng_type))) return kno_err("NotNNG",caller,NULL,arg); \
  else if (!(socketp((kno_nng)arg))) return kno_err("NotNNGSocket",caller,NULL,arg); \
  else NO_ELSE;

DEFC_PRIM("nng/close",nng_close_prim,
	     KNO_MAX_ARGS(1)|KNO_MIN_ARGS(1),
	     "Closes an NNG object",
	     {"ptr",kno_any_type,KNO_VOID})
static lispval nng_close_prim(lispval ptr)
{
  CHECK_NNG_TYPE(ptr,"nng/close");
  int rv = -1;
  struct KNO_NNG *ref = (kno_nng) ptr;
  switch (ref->nng_type) {
  case xnng_socket_type: case xnng_req0_type: case xnng_rep0_type:
  case xnng_pair0_type: case xnng_pair1_type:
  case xnng_pub0_type: case xnng_sub0_type: {
    if (nng_close(ref->nng_ptr.socket)) break;
    return KNO_TRUE;}

    CLOSE_CASE(dialer);
    CLOSE_CASE(listener);
    CLOSE_CASE(ctx);
    CLOSE_CASE(pipe);

  default:
    return kno_err("NotSupported","nng_close_prim",NULL,ptr);
  }
  if (rv) return mkerr(rv,"nng/close",ptr,NULL);
  else return KNO_TRUE;
}

DEFC_PRIM("nng/listen",nng_listen_prim,
	     KNO_MAX_ARGS(3)|KNO_MIN_ARGS(2),
	     "(NNG/LISTEN *sock* *url*)\n"
	     "Start listening on *url* using *sock*",
	     {"sock",kno_any_type,KNO_VOID},
	     {"url",kno_string_type,KNO_VOID},
	     {"opts",kno_any_type,KNO_FALSE})
static lispval nng_listen_prim(lispval sock,lispval url,lispval opts)
{
  CHECK_SOCKETP(sock,"nng/listen");
  struct KNO_NNG *l = kno_nng_create(xnng_listener_type);
  int rv = nng_listen(NNG_GET(sock,socket),KNO_CSTRING(url),
		      &(NNG_GET(l,listener)),0);
  if (rv) return mkerr(rv,"nng/listen",sock,KNO_CSTRING(url));
  else return LISPVAL(l);
}

DEFC_PRIM("nng/dial",nng_dial_prim,
	     KNO_MAX_ARGS(3)|KNO_MIN_ARGS(2),
	     "(NNG/DIAL *sock* *url*)\n"
	     "Start listening on *url* using *sock*",
	     {"sock",kno_any_type,KNO_VOID},
	     {"url",kno_string_type,KNO_VOID},
	     {"opts",kno_any_type,KNO_FALSE})
static lispval nng_dial_prim(lispval sock,lispval url,lispval opts)
{
  CHECK_SOCKETP(sock,"nng/dial");
  struct KNO_NNG *d = kno_nng_create(xnng_dialer_type);
  int rv = nng_dial(NNG_GET(sock,socket),KNO_CSTRING(url),
		    &(NNG_GET(d,dialer)),0);
  if (rv) return mkerr(rv,"nng/dial",sock,KNO_CSTRING(url));
  else return LISPVAL(d);
}

DEFC_PRIM("nng/send",nng_send_prim,
	     KNO_MAX_ARGS(3)|KNO_MIN_ARGS(2),
	     "(NNG/SEND *sock* *packet* [*opts*])\n"
	     "Send *packet* to *sock*",
	     {"sock",kno_any_type,KNO_VOID},
	     {"data",kno_any_type,KNO_VOID},
	     {"opts",kno_any_type,KNO_FALSE})
static lispval nng_send_prim(lispval sock,lispval data,lispval opts)
{
  CHECK_SOCKETP(sock,"nng/send");
  unsigned char * bytes; ssize_t len;
  if (KNO_STRINGP(data)) {
    bytes = (unsigned char *) KNO_CSTRING(data);
    len = KNO_STRLEN(data) + 1;}
  else if (KNO_PACKETP(data)) {
    bytes = (unsigned char *) KNO_PACKET_DATA(data);
    len = KNO_PACKET_LENGTH(data);}
  else return kno_err("NotStringOrPacket","nng/send",NULL,data);
  int rv = nng_send(NNG_GET(sock,socket),bytes,len,0);
  if (rv) return mkerr(rv,"nng/send",sock,NULL);
  else return kno_incref(sock);
}

DEFC_PRIM("nng/recv",nng_recv_prim,
	     KNO_MAX_ARGS(3)|KNO_MIN_ARGS(1),
	     "(NNG/RECV *sock* [*opts*] [*packet*])\n "
	     "Receive data from *sock*",
	     {"sock",kno_any_type,KNO_VOID},
	     {"opts",kno_any_type,KNO_VOID},
	     {"buf",kno_any_type,KNO_VOID})
static lispval nng_recv_prim(lispval sock,lispval opts,lispval buf)
{
  CHECK_SOCKETP(sock,"nng/recv");
  lispval result = KNO_VOID;
  unsigned char *bytes; ssize_t len;
  int rv = nng_recv(NNG_GET(sock,socket),&bytes,&len,NNG_FLAG_ALLOC);
  if (rv) return mkerr(rv,"nng/send",sock,NULL);
  else {
    lispval result = kno_make_packet(NULL,len,bytes);
    nng_free(bytes,len);
    return result;}
}

DEFC_PRIM("nng/pubsock",nng_pubsock_prim,
	     KNO_MAX_ARGS(0)|KNO_MIN_ARGS(0),
	     "Opens an NNG publish socket")
static lispval nng_pubsock_prim()
{
  struct KNO_NNG *ref = kno_nng_create(xnng_pub0_type);
  int rv = nng_pub0_open(&(ref->nng_ptr.socket));
  if (rv) return KNO_ERROR_VALUE;
  return LISPVAL(ref);
}

DEFC_PRIM("nng/subsock",nng_subsock_prim,
	     KNO_MAX_ARGS(0)|KNO_MIN_ARGS(0),
	     "Opens an NNG subscribe socket")
static lispval nng_subsock_prim()
{
  struct KNO_NNG *ref = kno_nng_create(xnng_sub0_type);
  int rv = nng_sub0_open(&(ref->nng_ptr.socket));
  if (rv) return KNO_ERROR_VALUE;
  return LISPVAL(ref);
}

DEFC_PRIM("nng/reqsock",nng_reqsock_prim,
	     KNO_MAX_ARGS(0)|KNO_MIN_ARGS(0),
	     "Opens an NNG request socket")
static lispval nng_reqsock_prim()
{
  struct KNO_NNG *ref = kno_nng_create(xnng_req0_type);
  int rv = nng_req0_open(&(ref->nng_ptr.socket));
  if (rv) return KNO_ERROR_VALUE;
  return LISPVAL(ref);
}

DEFC_PRIM("nng/repsock",nng_repsock_prim,
	     KNO_MAX_ARGS(0)|KNO_MIN_ARGS(0),
	     "Opens an NNG reply socket")
static lispval nng_repsock_prim()
{
  struct KNO_NNG *ref = kno_nng_create(xnng_rep0_type);
  int rv = nng_rep0_open(&(ref->nng_ptr.socket));
  if (rv) return KNO_ERROR_VALUE;
  return LISPVAL(ref);
}

DEFC_PRIM("nng/pair0",nng_pair0_prim,
	     KNO_MAX_ARGS(0)|KNO_MIN_ARGS(0),
	     "Opens an NNG pair0 socket")
static lispval nng_pair0_prim()
{
  struct KNO_NNG *ref = kno_nng_create(xnng_pair0_type);
  int rv = nng_pair0_open(&(ref->nng_ptr.socket));
  if (rv) return KNO_ERROR_VALUE;
  return LISPVAL(ref);
}

DEFC_PRIM("nng/pair1",nng_pair1_prim,
	     KNO_MAX_ARGS(0)|KNO_MIN_ARGS(0),
	     "Opens an NNG pair1 socket")
static lispval nng_pair1_prim()
{
  struct KNO_NNG *ref = kno_nng_create(xnng_pair1_type);
  int rv = nng_pair1_open(&(ref->nng_ptr.socket));
  if (rv) return KNO_ERROR_VALUE;
  return LISPVAL(ref);
}

/* Contexts */

DEFC_PRIM("nng/context",nng_getcontext,
	     KNO_MAX_ARGS(1)|KNO_MIN_ARGS(1),
	     "Allocates a context object for a socket",
	     {"socket",kno_any_type,KNO_VOID})
static lispval nng_getcontext(lispval socket)
{
  CHECK_SOCKETP(socket,"nng/getcontext");
  struct KNO_NNG *sref = (kno_nng) socket;
  nng_socket sock = sref->nng_ptr.socket;
  struct KNO_NNG *newref = kno_nng_create(xnng_ctx_type);
  int rv = nng_ctx_open(&(newref->nng_ptr.ctx),sock);
  if (rv) return KNO_ERROR_VALUE;
  return LISPVAL(newref);
}

/* Asynchronouse I/O */

void nng_handler_callback(void *obj)
{
  struct KNO_NNG *nng = obj;
  struct KNO_NNG_HANDLER *h = nng->nng_ptr.handler;
  lispval args[2] = { (lispval)h, h->state };
  lispval result = kno_apply(h->fcn,2,args);
  lispval old_state = h->state;
  h->state = result;
  kno_decref(old_state);
}

static lispval nng_handler(lispval fcn,
			   lispval data,
			   lispval init_state)
{
  if (!(KNO_APPLICABLEP(fcn)))
    return kno_err("NotApplicableFunction","nng_handler",NULL,fcn);
  struct KNO_NNG_HANDLER *handler = u8_alloc(struct KNO_NNG_HANDLER);
  handler->fcn   = fcn;
  handler->data  = data;
  handler->state = init_state;
  struct KNO_NNG *ref = kno_nng_create(xnng_handler_type);
  int rv = (ref) ?
    (nng_aio_alloc(&(handler->aio),
		   nng_handler_callback,
		   ref)) :
    (-1);
  if (rv == 0) {
    ref->nng_ptr.handler = handler;
    kno_incref(fcn);
    kno_incref(data);
    kno_incref(init_state);
    return (lispval) ref;}
  u8_free(handler);
  u8_free(ref);
  return KNO_ERROR;
}

static void aio_call_thunk(void *vdata)
{
  lispval thunk = (lispval)vdata;
  if (KNO_APPLICABLEP(thunk)) {
    lispval result = kno_apply(thunk,0,NULL);
    if (KNO_ABORTED(result)) {
      u8_exception ex = u8_erreify();
      if (ex==NULL)
	u8_log(LOG_ERR,"NNG/AIO/UndocumentedError",
	       "Error value without details from %q",
	       thunk);
      u8_log(LOG_ERR,"NNG/AIO/Error","%s <%s> (%s) from %q",
	     ex->u8x_cond,ex->u8x_context,ex->u8x_details,
	     thunk);
      u8_free_exception(ex,1);}
    kno_decref(result);}
  else u8_log(LOG_CRIT,"NNG/AIO/BadThunk","Can't apply %q",thunk);
}


DEFC_PRIM("nng/aio",nng_aio_prim,
	     KNO_MAX_ARGS(0)|KNO_MIN_ARGS(0),
	     "Opens an NNG AIO object")
static lispval nng_aio_prim(lispval callback)
{
  struct KNO_NNG *ref = kno_nng_create(xnng_ctx_type);
  int rv = nng_aio_alloc(&(ref->nng_ptr.aio),aio_call_thunk,
			 (void *)callback);
  if (rv) return KNO_ERROR_VALUE;
  kno_incref(callback);
  return LISPVAL(ref);
}

/* Services */

static void waiter_callback(void *arg);

struct KNO_NNG_SERVER *make_server(u8_string addr,int n_waiters,lispval handlers)
{
  struct KNO_NNG_SERVER *server = u8_alloc(struct KNO_NNG_SERVER);
  int flags = 0;
  int rv = nng_rep0_open(&(server->socket));
  if (rv) {}
  rv = nng_listen(server->socket,addr,&(server->listener),flags);
  if (rv) {}
  struct KNO_NNG_WAITER *waiters = u8_alloc_n(n_waiters,struct KNO_NNG_WAITER);
  int i = 0; while (i<n_waiters) {
    waiters[i].state = INIT;
    waiters[i].server = server;
    rv = nng_ctx_open(&(waiters[i].ctx),server->socket);
    if (rv) {}
    rv = nng_aio_alloc(&(waiters[i].aio),waiter_callback,&(waiters[i]));
    if (rv) {}
    i++;}
  server->waiters = waiters;
  server->n_waiters = n_waiters;
  server->handlers = kno_incref(handlers);
  // kno_init_xrefs(&(server->xrefs));
  return server;
}

static void waiter_callback(void *arg)
{
  struct KNO_NNG_WAITER *waiter = arg;
  struct KNO_NNG_SERVER *server = waiter->server;
  switch (waiter->state) {
  case INIT:
    waiter->state = RECV;
    nng_ctx_recv(waiter->ctx,waiter->aio);
    return;
  case RECV:
    if ( (nng_aio_result(waiter->aio))!=0 ) {}
    nng_msg *request = nng_aio_get_msg(waiter->aio);
    unsigned char *reqdata = nng_msg_body(request);
    ssize_t reqlen = nng_msg_len(request);
    lispval req = kno_decode_xtype(reqdata,reqlen,&(server->xrefs));
    nng_msg_free(request);
    waiter->request = req;
    lispval result = kno_exec(req,server->handlers,NULL);
    waiter->response = result;
    ssize_t response_len;
    unsigned char *response = kno_encode_xtype(result,&response_len,&(server->xrefs));
    int rv = nng_msg_alloc(&(waiter->response_msg),response_len);
    if (rv) {}
    rv = nng_msg_append(waiter->response_msg,response,response_len);
    nng_aio_set_msg(waiter->aio,waiter->response_msg);
    u8_free(response);
    nng_ctx_send(waiter->ctx,waiter->aio);
    waiter->state = SEND;
    return;
  case SEND:
    if ( (nng_aio_result(waiter->aio))!=0 ) {}
    nng_msg_free(waiter->response_msg); waiter->response_msg = NULL;
    kno_decref(waiter->request); waiter->request = KNO_VOID;
    kno_decref(waiter->response); waiter->response = KNO_VOID;
    nng_ctx_recv(waiter->ctx,waiter->aio);
    waiter->state = RECV;
    return;
  default:
    return;
  }
}

/* Initialization */

KNO_EXPORT int kno_init_nng(void) KNO_LIBINIT_FN;
static long long int nng_initialized = 0;

#define DEFAULT_FLAGS (KNO_SHORT2LISP(KNO_MONGODB_DEFAULTS))

static void init_nng_typemap(void);
static lispval nng_module;

KNO_EXPORT int kno_init_nng()
{
  if (nng_initialized) return 0;
  nng_initialized = u8_millitime();

  kno_nng_type = kno_register_cons_type("nng",KNO_NNG_TYPE);

  init_nng_typemap();

  nng_module = kno_new_cmodule("nng",0,kno_init_nng);
  link_local_cprims();

  return 1;
}

static void link_typecode(xnng_type t,u8_string s)
{
  lispval sym = kno_getsym(s), code = KNO_INT(((int)t));
  kno_store(nng_typemap,sym,code);
  kno_store(nng_typemap,code,sym);
}

static void init_nng_typemap()
{
  nng_typemap = kno_make_hashtable(NULL,512);

  link_typecode(xnng_socket_type,"socket");
  link_typecode(xnng_pub0_type,"pub0");
  link_typecode(xnng_sub0_type,"sub0");
  link_typecode(xnng_req0_type,"req0");
  link_typecode(xnng_rep0_type,"rep0");
  link_typecode(xnng_pub0_type,"pub");
  link_typecode(xnng_sub0_type,"sub");
  link_typecode(xnng_req0_type,"req");
  link_typecode(xnng_rep0_type,"rep");
  link_typecode(xnng_aio_type,"aio");
  link_typecode(xnng_ctx_type,"ctx");
  link_typecode(xnng_iov_type,"iov");
  link_typecode(xnng_msg_type,"msg");
  link_typecode(xnng_url_type,"url");
  link_typecode(xnng_pipe_type,"pipe");
  link_typecode(xnng_stat_type,"stat");
  link_typecode(xnng_dialer_type,"dialer");
  link_typecode(xnng_stream_type,"stream");
  link_typecode(xnng_pipe_ev_type,"pipe_ev");
  link_typecode(xnng_duration_type,"duration");
  link_typecode(xnng_listener_type,"listener");
  link_typecode(xnng_sockaddr_type,"sockaddr");
  link_typecode(xnng_sockaddr_in_type,"sockaddr_in");
  link_typecode(xnng_sockaddr_zt_type,"sockaddr_zt");
  link_typecode(xnng_sockaddr_in6_type,"sockaddr_in6");
  link_typecode(xnng_sockaddr_ipc_type,"sockaddr_ipc");
  link_typecode(xnng_sockaddr_tcp_type,"sockaddr_tcp");
  link_typecode(xnng_sockaddr_udp_type,"sockaddr_udp");
  link_typecode(xnng_sockaddr_path_type,"sockaddr_path");
  link_typecode(xnng_sockaddr_tcp6_type,"sockaddr_tcp6");
  link_typecode(xnng_sockaddr_udp6_type,"sockaddr_udp6");
  link_typecode(xnng_stream_dialer_type,"stream_dialer");
  link_typecode(xnng_sockaddr_inproc_type,"sockaddr_inproc");
  link_typecode(xnng_stream_listener_type,"stream_listener");

  link_typecode(xnng_socket_type,"nng_socket");
  link_typecode(xnng_handler_type,"nng_handler");
  link_typecode(xnng_aio_type,"nng_aio");
  link_typecode(xnng_pub0_type,"nng_pub0");
  link_typecode(xnng_sub0_type,"nng_sub0");
  link_typecode(xnng_req0_type,"nng_req0");
  link_typecode(xnng_rep0_type,"nng_rep0");
  link_typecode(xnng_aio_type,"nng_aio");
  link_typecode(xnng_ctx_type,"nng_ctx");
  link_typecode(xnng_iov_type,"nng_iov");
  link_typecode(xnng_msg_type,"nng_msg");
  link_typecode(xnng_url_type,"nng_url");
  link_typecode(xnng_pipe_type,"nng_pipe");
  link_typecode(xnng_stat_type,"nng_stat");
  link_typecode(xnng_dialer_type,"nng_dialer");
  link_typecode(xnng_stream_type,"nng_stream");
  link_typecode(xnng_pipe_ev_type,"nng_pipe_ev");
  link_typecode(xnng_duration_type,"nng_duration");
  link_typecode(xnng_listener_type,"nng_listener");
  link_typecode(xnng_sockaddr_type,"nng_sockaddr");
  link_typecode(xnng_sockaddr_in_type,"nng_sockaddr_in");
  link_typecode(xnng_sockaddr_zt_type,"nng_sockaddr_zt");
  link_typecode(xnng_sockaddr_in6_type,"nng_sockaddr_in6");
  link_typecode(xnng_sockaddr_ipc_type,"nng_sockaddr_ipc");
  link_typecode(xnng_sockaddr_tcp_type,"nng_sockaddr_tcp");
  link_typecode(xnng_sockaddr_udp_type,"nng_sockaddr_udp");
  link_typecode(xnng_sockaddr_path_type,"nng_sockaddr_path");
  link_typecode(xnng_sockaddr_tcp6_type,"nng_sockaddr_tcp6");
  link_typecode(xnng_sockaddr_udp6_type,"nng_sockaddr_udp6");
  link_typecode(xnng_stream_dialer_type,"nng_stream_dialer");
  link_typecode(xnng_sockaddr_inproc_type,"nng_sockaddr_inproc");
  link_typecode(xnng_stream_listener_type,"nng_stream_listener");

}

static void link_local_cprims()
{
  KNO_LINK_CPRIM("nng?",nngp_prim,2,nng_module);
  KNO_LINK_CPRIM("nng/close",nng_close_prim,1,nng_module);
  KNO_LINK_CPRIM("nng/pubsock",nng_pubsock_prim,0,nng_module);
  KNO_LINK_CPRIM("nng/subsock",nng_subsock_prim,0,nng_module);
  KNO_LINK_CPRIM("nng/reqsock",nng_reqsock_prim,0,nng_module);
  KNO_LINK_CPRIM("nng/repsock",nng_repsock_prim,0,nng_module);
  KNO_LINK_CPRIM("nng/pair0",nng_pair0_prim,0,nng_module);
  KNO_LINK_CPRIM("nng/pair1",nng_pair1_prim,0,nng_module);
  KNO_LINK_CPRIM("nng/listen",nng_listen_prim,3,nng_module);
  KNO_LINK_CPRIM("nng/dial",nng_dial_prim,3,nng_module);
  KNO_LINK_CPRIM("nng/send",nng_send_prim,3,nng_module);
  KNO_LINK_CPRIM("nng/recv",nng_recv_prim,3,nng_module);
  KNO_LINK_CPRIM("nng/getcontext",nng_getcontext,1,nng_module);
  KNO_LINK_CPRIM("nng/aio",nng_aio_prim,1,nng_module);
}
