(ns icfp2014.core
  (:require [icfp2014.compiler :as compiler]
            [icfp2014.ghc :as ghc]))

(defn -main
  "I do a whole lot.

  For every .llcoolj file: print compiled code to stdout AND write it to lambdaman.gcc.
 
  For every .gfk file: compile and output a corresponding .ghc file."
  [& files]
  (let [llcoolj-files (filter #(.endsWith ^String % "llcoolj") files)
        ghostface-files (filter #(.endsWith ^String % "gfk") files)]
    (when (seq llcoolj-files)
      (let [output (apply compiler/compile-files files)]
        (println output)
        (spit "lambdaman.gcc" output)))
    (when (seq ghostface-files)
      (doseq [file ghostface-files]
        (let [out-name (clojure.string/replace file "gfk" "ghc")]
          (ghc/tag-file file out-name)
          (println (format "%s -> %s" file out-name)))))))
