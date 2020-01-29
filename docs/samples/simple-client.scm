(use-module '{nng texttools logger webtools varconfig})

(define socket #f)

(define (connect-to url)
  (let* ((socket (nng/reqsock))
	 (dialer (nng/dial socket url)))
    (cons socket dialer)))

(define (reval expr (server socket))
  (cond ((and (pair? server) (nng? (car server)) (nng? (cdr server))))
	((string? server)
	 (set! server (connect-to server)))
	(else (error |NoServer|)))
  (nng/send (car server) (emit-xtype expr))
  (read-xtype (nng/recv (car server))))

(config-def! 'server
  (lambda (var (val))
    (cond ((not (bound? val)) socket)
	  ((nng? val) (set! socket val))
	  ((string? val) (set! socket (connect-to val)))
	  (else (irritant val |InvalidServerSpec|)))))

