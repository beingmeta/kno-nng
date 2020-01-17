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

static lispval nng_aio_symbol, nng_ctx_symbol, nng_iov_symbol, nng_msg_symbol,
  nng_url_symbol, nng_pipe_symbol, nng_stat_symbol, nng_dialer_symbol,
  nng_socket_symbol, nng_stream_symbol, nng_pipe_ev_symbol, 
  nng_duration_symbol, nng_listener_symbol, nng_sockaddr_symbol,
  nng_sockaddr_in_symbol, nng_sockaddr_zt_symbol, nng_sockaddr_in6_symbol, 
  nng_sockaddr_ipc_symbol, nng_sockaddr_tcp_symbol, nng_sockaddr_udp_symbol,
  nng_sockaddr_path_symbol, nng_sockaddr_tcp6_symbol, nng_sockaddr_udp6_symbol,
  nng_stream_dialer_symbol, nng_sockaddr_inproc_symbol, 
  nng_stream_listener_symbol;

static lispval get_typesym(kno_nng_type type)
{
  switch (type) {
  case kno_nng_aio_type: return KNOSYM(nng_aio);
  case kno_nng_ctx_type: return KNOSYM(nng_ctx);
  case kno_nng_iov_type: return KNOSYM(nng_iov);
  case kno_nng_msg_type: return KNOSYM(nng_msg);
  case kno_nng_url_type: return KNOSYM(nng_url);
  case kno_nng_pipe_type: return KNOSYM(nng_pipe);
  case kno_nng_stat_type: return KNOSYM(nng_stat);
  case kno_nng_dialer_type: return KNOSYM(nng_dialer);
  case kno_nng_socket_type: return KNOSYM(nng_socket);
  case kno_nng_stream_type: return KNOSYM(nng_stream);
  case kno_nng_pipe_ev_type: return KNOSYM(nng_pipe_ev);
  case kno_nng_duration_type: return KNOSYM(nng_duration);
  case kno_nng_listener_type: return KNOSYM(nng_listener);
  case kno_nng_sockaddr_type: return KNOSYM(nng_sockaddr);
  case kno_nng_sockaddr_in_type: return KNOSYM(nng_sockaddr_in);
  case kno_nng_sockaddr_zt_type: return KNOSYM(nng_sockaddr_zt);
  case kno_nng_sockaddr_in6_type: return KNOSYM(nng_sockaddr_in6);
  case kno_nng_sockaddr_ipc_type: return KNOSYM(nng_sockaddr_ipc);
  case kno_nng_sockaddr_tcp_type: return KNOSYM(nng_sockaddr_tcp);
  case kno_nng_sockaddr_udp_type: return KNOSYM(nng_sockaddr_udp);
  case kno_nng_sockaddr_path_type: return KNOSYM(nng_sockaddr_path);
  case kno_nng_sockaddr_tcp6_type: return KNOSYM(nng_sockaddr_tcp6);
  case kno_nng_sockaddr_udp6_type: return KNOSYM(nng_sockaddr_udp6);
  case kno_nng_stream_dialer_type: return KNOSYM(nng_stream_dialer);
  case kno_nng_sockaddr_inproc_type: return KNOSYM(nng_sockaddr_inproc);
  case kno_nng_stream_listener_type: return KNOSYM(nng_stream_listener);
  default: return KNO_FALSE;
  }
}

static kno_lisp_type kno_nngobj_type;

struct KNO_NNG *kno_nng_create(kno_nng_type type,lispval typesym)
{
  struct KNO_NNG *fresh = u8_alloc(struct KNO_NNG);
  KNO_INIT_FRESH_CONS(fresh,kno_nngobj_type);
  fresh->typetag = typesym;
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

static lispval close_prim(lispval ptr)
{
  struct KNO_NNG *ref = (kno_nng) ptr;
  
}


/* Initialization */

KNO_EXPORT int kno_init_knonng(void) KNO_LIBINIT_FN;
static long long int knonng_initialized = 0;

#define DEFAULT_FLAGS (KNO_SHORT2LISP(KNO_MONGODB_DEFAULTS))

static lispval knonng_module;

KNO_EXPORT int kno_init_knonng()
{
  if (knonng_initialized) return 0;
  knonng_initialized = u8_millitime();

  knonng_module = kno_new_cmodule("knonng",0,kno_init_knonng);
  return 1;
}

static void link_local_cprims()
{
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
