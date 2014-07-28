(ns icfp2014.core
  (:require [icfp2014.compiler :as compiler]))

(defn -main
  "I do a whole lot."
  [& files]
  (println (apply compiler/compile-files files)))
