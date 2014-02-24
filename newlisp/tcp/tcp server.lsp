; tcp server
(constant 'max-bytes 1024)

(if (not (set 'listen (net-listen 8888)))
	(println (net-error)))

(while (not (net-error))
	(set 'connection (net-accept listen)) ;block here
	(while (not (net-error))
		(net-receive connection message-from-client max-bytes)
		(println message-from-client)
		(net-send connection "hi from server")
	)

)