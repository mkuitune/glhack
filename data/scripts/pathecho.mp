;; Write file to path1 with path2 set as the arg param

(def argc (count sys/args))
(def usage "Usage:masp pathecho param1 param2 (you forgot a param.)")
(def pathname "asset_path")

(println (str "args:" sys/args))

;; Write config file to path1
(defn write_config (path1 path2)
    (println (str "You said " path1 " and " path2))
    (def msg
        (str "(def " pathname " \"" path1 "\")"))  
    (write path2 msg)
)

(if (< argc 4)
    (println usage)
    (write_config (sys/args 2) (sys/args 3))
)

