(use-module 'nng)

(define (get-repsock url)
  (let* ((s (nng/repsock))
	 (l (nng/listen s url)))
    (cons s l)))

(define (get-reqsock url)
  (let* ((s (nng/reqsock))
	 (l (nng/dial s url)))
    (cons s l)))

(define stopped #f)

(define (server-loop socket)
  (let* ((socket (nng/repsock))
	 (listener (nng/listen s url)))
    (while (not stopped)
      (let* ((packet (nng/recv socket))
	     (expr (read-xtype packet)))
	(if (eq? expr 'quit)
	    (set! stopped #t)
	    (onerror (let* ((value (eval expr))
			    (encoded (emit-xtype value)))
		       (nng/send socket encoded))
		(lambda (x)
		  (nng/send socket (emit-xtype 'error)))))))))

(define main server-loop)
