typedef struct nng_aio  nng_aio;
typedef struct nng_msg  nng_msg;
typedef struct nng_stat nng_stat;
typedef int32_t         nng_duration;
typedef struct nng_stream          nng_stream;
typedef struct nng_sockaddr_in nng_sockaddr_in;
typedef struct nng_sockaddr_zt nng_sockaddr_zt;
typedef struct nng_sockaddr_in nng_sockaddr_tcp;
typedef struct nng_sockaddr_in nng_sockaddr_udp;
typedef struct nng_sockaddr_in6 nng_sockaddr_in6;
typedef struct nng_sockaddr_in6 nng_sockaddr_tcp6;
typedef struct nng_sockaddr_in6 nng_sockaddr_udp6;
typedef struct nng_sockaddr_path nng_sockaddr_ipc;
typedef struct nng_ctx_s {
	uint32_t id;
} nng_ctx;
typedef struct nng_sockaddr_path nng_sockaddr_path;
typedef struct nng_pipe_s {
	uint32_t id;
} nng_pipe;
typedef struct nng_stream_dialer   nng_stream_dialer;
typedef struct nng_sockaddr_inproc nng_sockaddr_inproc;
typedef struct nng_stream_listener nng_stream_listener;
typedef struct nng_dialer_s {
	uint32_t id;
} nng_dialer;
typedef struct nng_socket_s {
	uint32_t id;
} nng_socket;
typedef void (*nng_aio_cancelfn)(nng_aio *, void *, int);
typedef void (*nng_pipe_cb)(nng_pipe, nng_pipe_ev, void *);
typedef struct nng_listener_s {
	uint32_t id;
} nng_listener;
typedef struct nng_iov {
	void * iov_buf;
	size_t iov_len;
} nng_iov;
typedef union nng_sockaddr {
	uint16_t            s_family;
	nng_sockaddr_ipc    s_ipc;
	nng_sockaddr_inproc s_inproc;
	nng_sockaddr_in6    s_in6;
	nng_sockaddr_in     s_in;
	nng_sockaddr_zt     s_zt;
} nng_sockaddr;
typedef enum {
	NNG_PIPE_EV_ADD_PRE,  // Called just before pipe added to socket
	NNG_PIPE_EV_ADD_POST, // Called just after pipe added to socket
	NNG_PIPE_EV_REM_POST, // Called just after pipe removed from socket
	NNG_PIPE_EV_NUM,      // Used internally, must be last.
} nng_pipe_ev;
typedef struct nng_url {
	char *u_rawurl;   // never NULL
	char *u_scheme;   // never NULL
	char *u_userinfo; // will be NULL if not specified
	char *u_host;     // including colon and port
	char *u_hostname; // name only, will be "" if not specified
	char *u_port;     // port, will be "" if not specified
	char *u_path;     // path, will be "" if not specified
	char *u_query;    // without '?', will be NULL if not specified
	char *u_fragment; // without '#', will be NULL if not specified
	char *u_requri;   // includes query and fragment, "" if not specified
} nng_url;
