;;; -*- Mode: Scheme; -*-

(in-module 'nng/server)

(use-module '{nng logger})

(define %loglevel %notice%)

(define stopped #f)

(define (server-loop (url (config 'server)))
  (let* ((socket (nng/repsock))
	 (listener (nng/listen socket url)))
    (while (not stopped)
      (let* ((packet (nng/recv socket))
	     (expr (read-xtype packet)))
	(logdebug |ReadPacket| packet)
	(loginfo |Read| expr)
	(if (eq? expr 'quit)
	    (set! stopped #t)
	    (onerror (let* ((value (eval expr))
			    (encoded (emit-xtype value)))
		       (loginfo |Result| value)
		       (nng/send socket encoded)
		       (logdebug |WrotePacket| encoded))
		(lambda (x)
		  (logerr |NNGError| x)
		  (nng/send socket (emit-xtype 'error)))))))))
