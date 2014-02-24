; tcp client

; client connect to server
(if (not (set 'connection (net-connect "localhost" 8888)))
	(println (net-error)))

; maximum bytes to receive
(constant 'max-bytes 1024)

;(while (not (net-error))
	(net-send connection "hi from client")
	(net-receive connection message-from-client max-bytes)
	(println message-from-client)
;)

