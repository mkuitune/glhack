(defn write (msg)
    (def ofile (OutputFile "out.txt"))
    (. 'write ofile msg)
    (. 'close ofile)
)

(defn writemore (msg)
    (def ofile (OutputFileApp "out.txt" true))
    (. 'write ofile msg)
    (. 'close ofile)
)


(write "Hello there.\nThis should start a new line.")
(writemore "\n\nDear me. I do hope I've not erased anything.")


