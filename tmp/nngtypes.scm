{"typedef struct nng_aio  nng_aio" "typedef struct nng_msg  nng_msg" "typedef struct nng_stat nng_stat" "typedef int32_t         nng_duration" "typedef struct nng_stream          nng_stream" "typedef struct nng_sockaddr_in nng_sockaddr_in" "typedef struct nng_sockaddr_zt nng_sockaddr_zt" "typedef struct nng_sockaddr_in nng_sockaddr_tcp" "typedef struct nng_sockaddr_in nng_sockaddr_udp" "typedef struct nng_sockaddr_in6 nng_sockaddr_in6" "typedef struct nng_sockaddr_in6 nng_sockaddr_tcp6" "typedef struct nng_sockaddr_in6 nng_sockaddr_udp6" "typedef struct nng_sockaddr_path nng_sockaddr_ipc" "typedef struct nng_ctx_s {\n\tuint32_t id;\n} nng_ctx" "typedef struct nng_sockaddr_path nng_sockaddr_path" "typedef struct nng_pipe_s {\n\tuint32_t id;\n} nng_pipe" "typedef struct nng_stream_dialer   nng_stream_dialer" "typedef struct nng_sockaddr_inproc nng_sockaddr_inproc" "typedef struct nng_stream_listener nng_stream_listener" "typedef struct nng_dialer_s {\n\tuint32_t id;\n} nng_dialer" "typedef struct nng_socket_s {\n\tuint32_t id;\n} nng_socket" "typedef void (*nng_aio_cancelfn)(nng_aio *, void *, int)" "typedef void (*nng_pipe_cb)(nng_pipe, nng_pipe_ev, void *)" "typedef struct nng_listener_s {\n\tuint32_t id;\n} nng_listener" "typedef struct nng_iov {\n\tvoid * iov_buf;\n\tsize_t iov_len;\n} nng_iov" "typedef union nng_sockaddr {\n\tuint16_t            s_family;\n\tnng_sockaddr_ipc    s_ipc;\n\tnng_sockaddr_inproc s_inproc;\n\tnng_sockaddr_in6    s_in6;\n\tnng_sockaddr_in     s_in;\n\tnng_sockaddr_zt     s_zt;\n} nng_sockaddr" "typedef enum {\n\tNNG_PIPE_EV_ADD_PRE,  // Called just before pipe added to socket\n\tNNG_PIPE_EV_ADD_POST, // Called just after pipe added to socket\n\tNNG_PIPE_EV_REM_POST, // Called just after pipe removed from socket\n\tNNG_PIPE_EV_NUM,      // Used internally, must be last.\n} nng_pipe_ev" "typedef struct nng_url {\n\tchar *u_rawurl;   // never NULL\n\tchar *u_scheme;   // never NULL\n\tchar *u_userinfo; // will be NULL if not specified\n\tchar *u_host;     // including colon and port\n\tchar *u_hostname; // name only, will be \"\" if not specified\n\tchar *u_port;     // port, will be \"\" if not specified\n\tchar *u_path;     // path, will be \"\" if not specified\n\tchar *u_query;    // without '?', will be NULL if not specified\n\tchar *u_fragment; // without '#', will be NULL if not specified\n\tchar *u_requri;   // includes query and fragment, \"\" if not specified\n} nng_url"}