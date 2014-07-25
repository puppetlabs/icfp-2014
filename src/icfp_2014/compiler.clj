(ns icfp_2014.compiler
  (:require [clojure.java.io]))

(def fns (atom {}))

(defn compile-form
  [symbols form]
  (cond
    (integer? form)
    [[:ldc form]]

    (symbol? form)
    (if (= :fn (:type (symbols form)))
      [[:ldf form]]
      [[:ld 0 (:index (symbols form))]])

    (vector? form)
    (if (< (count form) 2)
      (throw (IllegalArgumentException. (format "can't create vector of < 2 elements: %s" form)))
      (if (some vector? form)
        (throw (IllegalArgumentException. (format "can't create vector of vectors: %s" form)))
        (let [twiddled-vec (concat (take (- (count form) 2) form)
                                   (take 2 (reverse form)))]
          (concat (mapcat #(compile-form symbols %) twiddled-vec)
                  (repeat (dec (count form)) [:cons])))))

    (list? form)
    (let [[fn-name & args] form
          load-args ]
      (cons [:ap (dec (count form))]
            ;; Push the args onto the stack in reverse order:
            ;; first arg is top of the stack
            (mapcat #(compile-form symbols %) args)))))

(defn compile-ai
  [file]
  (let [prog (java.io.PushbackReader. (clojure.java.io/reader file))]
    (loop [form (read prog)
           symbols {}]
      (let [fn-name (name (first form))
            args (second form)
            forms (drop 2 form)
            symbols (conj symbols fn-name {:type :fn :index (count symbols)})]
        (mapcat #(compile-form symbols %) forms)))))
