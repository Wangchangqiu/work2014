(define (clean-folder dir-path)
	(println "enter " dir-path)
	(let (fs (directory dir-path "\\.LOG|\\.TMP"))
		(dolist (af fs)
			(begin
				(println (append "remove files: " dir-path "\\" af))
				(delete-file (append dir-path "\\" af))
			)
		)
	)

	(let (files (directory dir-path))
		(begin
			(dolist (f files)
				(let (cf (append dir-path "\\" f))	
					(and (if (directory? cf)) (if (!= f ".")) (if (!= f ".."))
							(begin
								(println (append "remove folder: " cf))
								(clean-folder cf)
							)
					)
				)
			)	
		)
	)
)

(clean-folder "c:\\test")

