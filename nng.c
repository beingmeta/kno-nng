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

static lispval get_typesym(kno_nng_type type)
{
  lispval key = KNO_INT(((int)type));
  return kno_hashtable_get((kno_hashtable)nng_typemap,key,KNO_FALSE);
}

static kno_lisp_type kno_nngobj_type;

struct KNO_NNG *kno_nng_create(kno_nng_type type)
{
  struct KNO_NNG *fresh = u8_alloc(struct KNO_NNG);
  KNO_INIT_FRESH_CONS(fresh,kno_nngobj_type);
  fresh->typetag = get_typesym(type);
  fresh->typeinfo = NULL;
  fresh->nng_type = type;
  return fresh;
}

static lispval pubsock_symbol;

static lispval pub_open()
{
  struct KNO_NNG *ref = kno_nng_create(kno_nng_socket_type,KNOSYM(pubsock));
  int rv = pub_open(&(ref->nng_ptr.socket));
  if (rv) return KNO_ERROR_VALUE;
  return LISPVAL(ref);
}

static lispval nng_kno_err(u8_context cxt,lispval obj)
{
  return kno_err("NNG_Error",cxt,NULL,obj);
}

#define CLOSE_CASE(typename) \
  case kno_nng_ ## typename ## _type: \
  if (nng_ ## typename ## _close(ref->nng_ptr.nng_ ## typename)) break; \
  return KNO_TRUE


KNO_DEFPRIM1("nng/close",nng_close_prim,KNO_MIN_ARGS(1),
	     "Closes an NNG object",
	     kno_nng_type,KNO_VOID)
static lispval ngg_close_prim(lispval ptr)
{
  struct KNO_NNG *ref = (kno_nng) ptr;
  switch (ref->nng_type) {
  case kno_nng_socket_type:
    if (nng_close(ref->nng_ptr.nng_socket)) break;
    return KNO_TRUE;
    CLOSE_CASE(dialer);
    CLOSE_CASE(listener);
    CLOSE_CASE(ctx);
    CLOSE_CASE(pipe);
    CLOSE_CASE(stream);
    CLOSE_CASE(stream_dialer);
    CLOSE_CASE(stream_listener);
  default:
    return kno_err("NotSupported","ngg_close_prim",NULL,ptr);
  }
  return nng_err("close",ptr);
}

KNO_DEFPRIM1("nng/dialer",nng_dialer_prim,KNO_MIN_ARGS(1),
	     "Opens an NNG dialer",
	     kno_string_type_type,KNO_VOID)
static lispval nng_dialer_prim(lispval spec)
{
  struct KNO_NNG *dialer = kno_nng_create(kno_nng_dialer_type);
  nng_socket sock = 3;
  int rv = nng_dialer_create(&(dialer->nng_ptr.nng_dialer),sock,
			     KNO_CSTRING(spec));
  if (rv) {
    u8_free(dialer);
    return nng_err("nng/dialer",VOID);}
  return LISPVAL(dialer);
}

/* Initialization */

KNO_EXPORT int kno_init_nng(void) KNO_LIBINIT_FN;
static long long int nng_initialized = 0;

#define DEFAULT_FLAGS (KNO_SHORT2LISP(KNO_MONGODB_DEFAULTS))

static lispval nng_module;

KNO_EXPORT int kno_init_nng()
{
  if (nng_initialized) return 0;
  knonng_initialized = u8_millitime();

  init_nng_typemap();

  nng_module = kno_new_cmodule("nng",0,kno_init_nng);
  return 1;
}

static void link_typecode(kno_nng_type t,u8_string s)
{
  lispval sym = kno_getsym(s), code = KNO_INT(((int)t));
  kno_store(nng_typemap,sym,code);
  kno_store(nng_typemap,code,sym);
}

static void init_nng_typemap()
{
  nng_typemap = kno_make_hashtable(NULL,512);

  link_typecode(kno_nng_socket_type,"socket");
  link_typecode(kno_nng_pub0_type,"pub0");
  link_typecode(kno_nng_sub0_type,"sub0");
  link_typecode(kno_nng_req0_type,"req0");
  link_typecode(kno_nng_rep0_type,"rep0");
  link_typecode(kno_nng_pub0_type,"pub");
  link_typecode(kno_nng_sub0_type,"sub");
  link_typecode(kno_nng_req0_type,"req");
  link_typecode(kno_nng_rep0_type,"rep");
  link_typecode(kno_nng_aio_type,"aio");
  link_typecode(kno_nng_ctx_type,"ctx");
  link_typecode(kno_nng_iov_type,"iov");
  link_typecode(kno_nng_msg_type,"msg");
  link_typecode(kno_nng_url_type,"url");
  link_typecode(kno_nng_pipe_type,"pipe");
  link_typecode(kno_nng_stat_type,"stat");
  link_typecode(kno_nng_dialer_type,"dialer");
  link_typecode(kno_nng_stream_type,"stream");
  link_typecode(kno_nng_pipe_ev_type,"pipe_ev");
  link_typecode(kno_nng_duration_type,"duration");
  link_typecode(kno_nng_listener_type,"listener");
  link_typecode(kno_nng_sockaddr_type,"sockaddr");
  link_typecode(kno_nng_sockaddr_in_type,"sockaddr_in");
  link_typecode(kno_nng_sockaddr_zt_type,"sockaddr_zt");
  link_typecode(kno_nng_sockaddr_in6_type,"sockaddr_in6");
  link_typecode(kno_nng_sockaddr_ipc_type,"sockaddr_ipc");
  link_typecode(kno_nng_sockaddr_tcp_type,"sockaddr_tcp");
  link_typecode(kno_nng_sockaddr_udp_type,"sockaddr_udp");
  link_typecode(kno_nng_sockaddr_path_type,"sockaddr_path");
  link_typecode(kno_nng_sockaddr_tcp6_type,"sockaddr_tcp6");
  link_typecode(kno_nng_sockaddr_udp6_type,"sockaddr_udp6");
  link_typecode(kno_nng_stream_dialer_type,"stream_dialer");
  link_typecode(kno_nng_sockaddr_inproc_type,"sockaddr_inproc");
  link_typecode(kno_nng_stream_listener_type,"stream_listener");

  link_typecode(kno_nng_socket_type,"nng_socket");
  link_typecode(kno_nng_pub0_type,"nng_pub0");
  link_typecode(kno_nng_sub0_type,"nng_sub0");
  link_typecode(kno_nng_req0_type,"nng_req0");
  link_typecode(kno_nng_rep0_type,"nng_rep0");
  link_typecode(kno_nng_aio_type,"nng_aio");
  link_typecode(kno_nng_ctx_type,"nng_ctx");
  link_typecode(kno_nng_iov_type,"nng_iov");
  link_typecode(kno_nng_msg_type,"nng_msg");
  link_typecode(kno_nng_url_type,"nng_url");
  link_typecode(kno_nng_pipe_type,"nng_pipe");
  link_typecode(kno_nng_stat_type,"nng_stat");
  link_typecode(kno_nng_dialer_type,"nng_dialer");
  link_typecode(kno_nng_stream_type,"nng_stream");
  link_typecode(kno_nng_pipe_ev_type,"nng_pipe_ev");
  link_typecode(kno_nng_duration_type,"nng_duration");
  link_typecode(kno_nng_listener_type,"nng_listener");
  link_typecode(kno_nng_sockaddr_type,"nng_sockaddr");
  link_typecode(kno_nng_sockaddr_in_type,"nng_sockaddr_in");
  link_typecode(kno_nng_sockaddr_zt_type,"nng_sockaddr_zt");
  link_typecode(kno_nng_sockaddr_in6_type,"nng_sockaddr_in6");
  link_typecode(kno_nng_sockaddr_ipc_type,"nng_sockaddr_ipc");
  link_typecode(kno_nng_sockaddr_tcp_type,"nng_sockaddr_tcp");
  link_typecode(kno_nng_sockaddr_udp_type,"nng_sockaddr_udp");
  link_typecode(kno_nng_sockaddr_path_type,"nng_sockaddr_path");
  link_typecode(kno_nng_sockaddr_tcp6_type,"nng_sockaddr_tcp6");
  link_typecode(kno_nng_sockaddr_udp6_type,"nng_sockaddr_udp6");
  link_typecode(kno_nng_stream_dialer_type,"nng_stream_dialer");
  link_typecode(kno_nng_sockaddr_inproc_type,"nng_sockaddr_inproc");
  link_typecode(kno_nng_stream_listener_type,"nng_stream_listener");

}

static void link_local_cprims()
{
  KNO_LINK_PRIM("nng/close",nng_close_prim,1,nng_module);
  KNO_LINK_PRIM("nng/listener",nng_listener_prim,1,nng_module);
#if 0
  KNO_LINK_PRIM("mongodb/dbinfo",mongodb_getinfo,2,mongodb_module);
  KNO_LINK_PRIM("mongodb/cursor?",mongodb_cursorp,1,mongodb_module);
  KNO_LINK_PRIM("mongodb/collection?",mongodb_collectionp,1,mongodb_module);
  KNO_LINK_PRIM("mongodb?",mongodbp,1,mongodb_module);
  KNO_LINK_PRIM("mongodb/getcollection",mongodb_getcollection,1,mongodb_module);
  KNO_LINK_PRIM("collection/name",mongodb_collection_name,1,mongodb_module);
  KNO_LINK_PRIM("mongodb/getdb",mongodb_getdb,1,mongodb_module);
  KNO_LINK_PRIM("mongodb/getopts",mongodb_getopts,1,mongodb_module);
  KNO_LINK_PRIM("mongodb/dburi",mongodb_uri,1,mongodb_module);
  KNO_LINK_PRIM("mongodb/dbspec",mongodb_spec,1,mongodb_module);
  KNO_LINK_PRIM("mongodb/dbname",mongodb_dbname,1,mongodb_module);
  KNO_LINK_PRIM("mongovec?",mongovecp,1,mongodb_module);
  KNO_LINK_PRIM("->mongovec",make_mongovec,1,mongodb_module);
  KNO_LINK_VARARGS("mongovec",mongovec_lexpr,mongodb_module);
  KNO_LINK_PRIM("cursor/readvec",mongodb_cursor_read_vector,3,mongodb_module);
  KNO_LINK_PRIM("cursor/read",mongodb_cursor_read,3,mongodb_module);
  KNO_LINK_PRIM("cursor/skip",mongodb_skip,2,mongodb_module);
  KNO_LINK_PRIM("cursor/done?",mongodb_donep,1,mongodb_module);
  KNO_LINK_PRIM("cursor/open",mongodb_cursor,3,mongodb_module);
  KNO_LINK_PRIM("cursor/open",mongodb_cursor,3,mongodb_module);
  KNO_LINK_VARARGS("mongodb/cmd",mongodb_simple_command,mongodb_module);
  KNO_LINK_VARARGS("mongodb/results",mongodb_command,mongodb_module);
  KNO_LINK_PRIM("->mongovec",make_mongovec,1,mongodb_module);
  KNO_LINK_PRIM("collection/modify!",mongodb_modify,4,mongodb_module);
  KNO_LINK_PRIM("collection/get",mongodb_get,3,mongodb_module);
  KNO_LINK_PRIM("collection/get",mongodb_get,3,mongodb_module);
  KNO_LINK_PRIM("collection/count",mongodb_count,3,mongodb_module);
  KNO_LINK_PRIM("collection/count",mongodb_count,3,mongodb_module);
  KNO_LINK_PRIM("collection/find",mongodb_find,3,mongodb_module);
  KNO_LINK_PRIM("collection/find",mongodb_find,3,mongodb_module);
  KNO_LINK_PRIM("collection/upsert!",mongodb_upsert,4,mongodb_module);
  KNO_LINK_PRIM("collection/update!",mongodb_update,4,mongodb_module);
  KNO_LINK_PRIM("collection/remove!",mongodb_remove,3,mongodb_module);
  KNO_LINK_PRIM("collection/insert!",mongodb_insert,3,mongodb_module);
  KNO_LINK_PRIM("collection/insert!",mongodb_insert,3,mongodb_module);
  KNO_LINK_PRIM("collection/open",mongodb_collection,3,mongodb_module);
  KNO_LINK_PRIM("mongodb/open",mongodb_open,2,mongodb_module);
  KNO_LINK_PRIM("mongodb/oid",mongodb_oidref,1,mongodb_module);

  KNO_LINK_TYPED("collection/remove!",mongodb_remove,3,mongodb_module,
		 kno_mongoc_collection,KNO_VOID,kno_any_type,KNO_VOID,
		 kno_any_type,KNO_VOID);
  KNO_LINK_TYPED("collection/update!",mongodb_update,4,mongodb_module,
		 kno_mongoc_collection,KNO_VOID,kno_any_type,KNO_VOID,
		 kno_any_type,KNO_VOID,kno_any_type,KNO_VOID);
  KNO_LINK_TYPED("collection/upsert!",mongodb_upsert,4,mongodb_module,
		 kno_mongoc_collection,KNO_VOID,kno_any_type,KNO_VOID,
		 kno_any_type,KNO_VOID,kno_any_type,KNO_VOID);
  KNO_LINK_TYPED("collection/find",mongodb_find,3,mongodb_module,
		 kno_mongoc_collection,KNO_VOID,kno_any_type,KNO_VOID,
		 kno_any_type,KNO_VOID);
  KNO_LINK_TYPED("collection/find",mongodb_find,3,mongodb_module,
		 kno_mongoc_collection,KNO_VOID,kno_any_type,KNO_VOID,
		 kno_any_type,KNO_VOID);
  KNO_LINK_TYPED("collection/count",mongodb_count,3,mongodb_module,
		 kno_mongoc_collection,KNO_VOID,kno_any_type,KNO_VOID,
		 kno_any_type,KNO_VOID);
  KNO_LINK_TYPED("collection/count",mongodb_count,3,mongodb_module,
		 kno_mongoc_collection,KNO_VOID,kno_any_type,KNO_VOID,
		 kno_any_type,KNO_VOID);
  KNO_LINK_TYPED("collection/get",mongodb_get,3,mongodb_module,
		 kno_mongoc_collection,KNO_VOID,kno_any_type,KNO_VOID,
		 kno_any_type,KNO_VOID);
  KNO_LINK_TYPED("collection/get",mongodb_get,3,mongodb_module,
		 kno_mongoc_collection,KNO_VOID,kno_any_type,KNO_VOID,
		 kno_any_type,KNO_VOID);
  KNO_LINK_TYPED("collection/modify!",mongodb_modify,4,mongodb_module,
		 kno_mongoc_collection,KNO_VOID,kno_any_type,KNO_VOID,
		 kno_any_type,KNO_VOID,kno_any_type,KNO_VOID);

  KNO_LINK_TYPED("cursor/done?",mongodb_donep,1,mongodb_module,
		 kno_mongoc_cursor,KNO_VOID);
  KNO_LINK_TYPED("cursor/skip",mongodb_skip,2,mongodb_module,
		 kno_mongoc_cursor,KNO_VOID,kno_fixnum_type,KNO_CPP_INT(1));


  KNO_LINK_TYPED("cursor/read",mongodb_cursor_read,3,mongodb_module,
		 kno_mongoc_cursor,KNO_VOID,kno_fixnum_type,KNO_CPP_INT(1),
		 kno_any_type,KNO_VOID);
  KNO_LINK_TYPED("cursor/readvec",mongodb_cursor_read_vector,3,mongodb_module,
		 kno_mongoc_cursor,KNO_VOID,kno_fixnum_type,KNO_CPP_INT(1),
		 kno_any_type,KNO_VOID);
  KNO_LINK_TYPED("mongodb/dbinfo",mongodb_getinfo,2,mongodb_module,
		 kno_mongoc_server,KNO_VOID,kno_any_type,KNO_VOID);

  KNO_LINK_ALIAS("mongo/oid",mongodb_oidref,mongodb_module);
  KNO_LINK_ALIAS("mongo/open",mongodb_open,mongodb_module);
  KNO_LINK_ALIAS("mongodb/collection",mongodb_collection,mongodb_module);
  KNO_LINK_ALIAS("mongo/collection",mongodb_collection,mongodb_module);
  KNO_LINK_ALIAS("mongo/insert!",mongodb_insert,mongodb_module);
  KNO_LINK_ALIAS("mongo/insert!",mongodb_insert,mongodb_module);
  KNO_LINK_ALIAS("mongo/remove!",mongodb_remove,mongodb_module);
  KNO_LINK_ALIAS("mongo/update!",mongodb_update,mongodb_module);
  KNO_LINK_ALIAS("mongo/find",mongodb_find,mongodb_module);
  KNO_LINK_ALIAS("mongo/find",mongodb_find,mongodb_module);
  KNO_LINK_ALIAS("mongo/get",mongodb_get,mongodb_module);
  KNO_LINK_ALIAS("mongo/get",mongodb_get,mongodb_module);
  KNO_LINK_ALIAS("collection/modify",mongodb_modify,mongodb_module);
  KNO_LINK_ALIAS("mongo/modify",mongodb_modify,mongodb_module);
  KNO_LINK_ALIAS("mongo/modify!",mongodb_modify,mongodb_module);
  KNO_LINK_ALIAS("mongo/results",mongodb_command,mongodb_module);
  KNO_LINK_ALIAS("mongodb/cursor",mongodb_cursor,mongodb_module);
  KNO_LINK_ALIAS("mongo/cursor",mongodb_cursor,mongodb_module);
  KNO_LINK_ALIAS("mongodb/cursor",mongodb_cursor,mongodb_module);
  KNO_LINK_ALIAS("mongo/cursor",mongodb_cursor,mongodb_module);
  KNO_LINK_ALIAS("mongo/done?",mongodb_donep,mongodb_module);
  KNO_LINK_ALIAS("mongo/skip",mongodb_skip,mongodb_module);

  KNO_LINK_ALIAS("mongo/read",mongodb_cursor_read,mongodb_module);
  KNO_LINK_ALIAS("mongo/read->vector",mongodb_cursor_read_vector,mongodb_module);
  KNO_LINK_ALIAS("mongo/name",mongodb_dbname,mongodb_module);
  KNO_LINK_ALIAS("mongodb/spec",mongodb_spec,mongodb_module);
  KNO_LINK_ALIAS("mongo/spec",mongodb_spec,mongodb_module);
  KNO_LINK_ALIAS("mongodb/uri",mongodb_uri,mongodb_module);
  KNO_LINK_ALIAS("mongo/uri",mongodb_uri,mongodb_module);
  KNO_LINK_ALIAS("mongodb/opts",mongodb_getopts,mongodb_module);
  KNO_LINK_ALIAS("mongo/opts",mongodb_getopts,mongodb_module);
  KNO_LINK_ALIAS("mongo/getdb",mongodb_getdb,mongodb_module);

  KNO_LINK_ALIAS("mongo/info",mongodb_getinfo,mongodb_module);
  KNO_LINK_ALIAS("mongo/getcollection",mongodb_getcollection,mongodb_module);
  KNO_LINK_ALIAS("mongo?",mongodbp,mongodb_module);

  KNO_LINK_ALIAS("collection?",mongodb_collectionp,mongodb_module);
  KNO_LINK_ALIAS("mongo/collection?",mongodb_collectionp,mongodb_module);
  KNO_LINK_ALIAS("cursor?",mongodb_cursorp,mongodb_module);
  KNO_LINK_ALIAS("mongo/cursor?",mongodb_cursorp,mongodb_module);
#endif
}
