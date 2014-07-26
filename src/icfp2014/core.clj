(ns icfp2014.core
  (:require [icfp2014.compiler :as compiler]))

(defn -main
  "I do a whole lot."
  [file & args]
  (println (compiler/compile-ai file)))
