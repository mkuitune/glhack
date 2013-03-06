(println "Number of input arguments is:")
(println (str "Count:" (count sys/args)))
(println "Arguments")
(iter '_ 'y sys/args 
    (fn (x) (println (str "value:" x))))
(sys/args 1)
(sys/args 2)
(sys/args 3)

