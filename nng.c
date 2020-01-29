/* -*- Mode: C; Character-encoding: utf-8; -*- */

/* mongodb.c
   This implements Kno bindings to mongodb.
   Copyright (C) 2007-2019 beingmeta, inc.
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
  nng_socket_symbol, nng_stream_symbol, nng_pipe_ev_symbol, 
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

struct KNO_NNG *kno_nng_create(xnng_type type)
{
  struct KNO_NNG *fresh = u8_alloc(struct KNO_NNG);
  KNO_INIT_FRESH_CONS(fresh,kno_nng_type);
  fresh->typetag = get_typesym(type);
  fresh->typeinfo = NULL;
  fresh->nng_type = type;
  return fresh;
}

static int unparse_nng_wrapper(u8_output out,lispval x)
{
  struct KNO_NNG *nng = (kno_nng) x;
  u8_printf(out,"#<NNG %q #!0x%lx>",nng->typetag,
	    ((unsigned long long)x));
  return 1;
}

KNO_DEFPRIM2("nng?",nngp_prim,KNO_MIN_ARGS(1),
	     "`(NNG? *obj* [*tag*]) returns #t if *obj* is "
	     "an NNG object and has the typetag *tag* (if provided)",
	     -1,KNO_VOID,kno_symbol_type,KNO_VOID)
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


KNO_DEFPRIM1("nng/close",nng_close_prim,KNO_MIN_ARGS(1),
	     "Closes an NNG object",
	     -1,KNO_VOID)
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

KNO_DEFPRIM3("nng/listen",nng_listen_prim,KNO_MIN_ARGS(2),
	     "(NNG/LISTEN *sock* *url*)\nStart listening on *url* using *sock*",
	     -1,KNO_VOID,kno_string_type,KNO_VOID,-1,KNO_FALSE)
static lispval nng_listen_prim(lispval sock,lispval url,lispval opts)
{
  CHECK_SOCKETP(sock,"nng/listen");
  struct KNO_NNG *l = kno_nng_create(xnng_listener_type);  
  int rv = nng_listen(NNG_GET(sock,socket),KNO_CSTRING(url),
		      &(NNG_GET(l,listener)),0);
  if (rv) return mkerr(rv,"nng/listen",sock,KNO_CSTRING(url));
  else return LISPVAL(l);
}

KNO_DEFPRIM3("nng/dial",nng_dial_prim,KNO_MIN_ARGS(2),
	     "(NNG/DIAL *sock* *url*)\nStart listening on *url* using *sock*",
	     -1,KNO_VOID,kno_string_type,KNO_VOID,-1,KNO_FALSE)
static lispval nng_dial_prim(lispval sock,lispval url,lispval opts)
{
  CHECK_SOCKETP(sock,"nng/dial");
  struct KNO_NNG *d = kno_nng_create(xnng_dialer_type);  
  int rv = nng_dial(NNG_GET(sock,socket),KNO_CSTRING(url),
		    &(NNG_GET(d,dialer)),0);
  if (rv) return mkerr(rv,"nng/dial",sock,KNO_CSTRING(url));
  else return LISPVAL(d);
}

KNO_DEFPRIM3("nng/send",nng_send_prim,KNO_MIN_ARGS(2),
	     "(NNG/SEND *sock* *packet* [*opts*])\nSend *packet* to *sock*",
	     -1,KNO_VOID,-1,KNO_VOID,-1,KNO_FALSE)
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

KNO_DEFPRIM3("nng/recv",nng_recv_prim,KNO_MIN_ARGS(1),
	     "(NNG/RECV *sock* [*opts*] [*packet*])\n Receive data from *sock*",
	     -1,KNO_VOID,-1,KNO_VOID,-1,KNO_VOID)
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

KNO_DEFPRIM("nng/pubsock",nng_pubsock_prim,KNO_MIN_ARGS(0),
	    "Opens an NNG publish socket")
static lispval nng_pubsock_prim()
{
  struct KNO_NNG *ref = kno_nng_create(xnng_pub0_type);
  int rv = nng_pub0_open(&(ref->nng_ptr.socket));
  if (rv) return KNO_ERROR_VALUE;
  return LISPVAL(ref);
}

KNO_DEFPRIM("nng/subsock",nng_subsock_prim,KNO_MIN_ARGS(0),
	    "Opens an NNG subscribe socket")
static lispval nng_subsock_prim()
{
  struct KNO_NNG *ref = kno_nng_create(xnng_sub0_type);
  int rv = nng_sub0_open(&(ref->nng_ptr.socket));
  if (rv) return KNO_ERROR_VALUE;
  return LISPVAL(ref);
}

KNO_DEFPRIM("nng/reqsock",nng_reqsock_prim,KNO_MIN_ARGS(0),
	    "Opens an NNG request socket")
static lispval nng_reqsock_prim()
{
  struct KNO_NNG *ref = kno_nng_create(xnng_req0_type);
  int rv = nng_req0_open(&(ref->nng_ptr.socket));
  if (rv) return KNO_ERROR_VALUE;
  return LISPVAL(ref);
}

KNO_DEFPRIM("nng/repsock",nng_repsock_prim,KNO_MIN_ARGS(0),
	    "Opens an NNG reply socket")
static lispval nng_repsock_prim()
{
  struct KNO_NNG *ref = kno_nng_create(xnng_rep0_type);
  int rv = nng_rep0_open(&(ref->nng_ptr.socket));
  if (rv) return KNO_ERROR_VALUE;
  return LISPVAL(ref);
}

KNO_DEFPRIM("nng/pair0",nng_pair0_prim,KNO_MIN_ARGS(0),
	    "Opens an NNG pair0 socket")
static lispval nng_pair0_prim()
{
  struct KNO_NNG *ref = kno_nng_create(xnng_pair0_type);
  int rv = nng_pair0_open(&(ref->nng_ptr.socket));
  if (rv) return KNO_ERROR_VALUE;
  return LISPVAL(ref);
}

KNO_DEFPRIM("nng/pair1",nng_pair1_prim,KNO_MIN_ARGS(0),
	    "Opens an NNG pair1 socket")
static lispval nng_pair1_prim()
{
  struct KNO_NNG *ref = kno_nng_create(xnng_pair1_type);
  int rv = nng_pair1_open(&(ref->nng_ptr.socket));
  if (rv) return KNO_ERROR_VALUE;
  return LISPVAL(ref);
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

  kno_nng_type = kno_register_cons_type("nng");
  kno_unparsers[kno_nng_type] = unparse_nng_wrapper;

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
  KNO_LINK_PRIM("nng?",nngp_prim,2,nng_module);
  KNO_LINK_PRIM("nng/close",nng_close_prim,1,nng_module);
  KNO_LINK_PRIM("nng/pubsock",nng_pubsock_prim,0,nng_module);
  KNO_LINK_PRIM("nng/subsock",nng_subsock_prim,0,nng_module);
  KNO_LINK_PRIM("nng/reqsock",nng_reqsock_prim,0,nng_module);
  KNO_LINK_PRIM("nng/repsock",nng_repsock_prim,0,nng_module);
  KNO_LINK_PRIM("nng/pair0",nng_pair0_prim,0,nng_module);
  KNO_LINK_PRIM("nng/pair1",nng_pair1_prim,0,nng_module);
  KNO_LINK_PRIM("nng/listen",nng_listen_prim,3,nng_module);
  KNO_LINK_PRIM("nng/dial",nng_dial_prim,3,nng_module);
  KNO_LINK_PRIM("nng/send",nng_send_prim,3,nng_module);
  KNO_LINK_PRIM("nng/recv",nng_recv_prim,3,nng_module);
}
