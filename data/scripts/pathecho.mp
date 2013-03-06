;; Write file to path1 with path2 set as the arg param

(def argc (count sys/args))
(def usage "Usage:masp pathecho param1 param2 (you forgot the param.)")
(def pathname "asset_path")

(defn write_file (path1 path2)
    (def msg
        (str "(def " pathname " \"" path2 "\")"))  
    (write path1 msg)
)

(if (< argc 4) (println usage)
   (begin
        (println (str "You said " (sys/args 2) " and " (sys/args 3)))
        (write_file (sys/args 2) (sys/args 3))
   ) 
)
