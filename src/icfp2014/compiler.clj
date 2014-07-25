(ns icfp2014.compiler
  (:require [clojure.java.io]
            [clojure.string :as string]))

(def fns (atom {}))

(defn compile-form
  [vars fns form]
  {:post [(or (prn "val is" %) (sequential? %))
          (every? vector? %)]}
  (cond
    (integer? form)
    [[:ldc form]]

    (symbol? form)
    (if (fns form)
      [[:ldf form]]
      [[:ld 0 (.indexOf vars form)]])

    (vector? form)
    (if (empty? form)
      [[:ldc 0]]
      (concat (mapcat #(compile-form vars fns %) form)
              [[:ldc 0]]
              (repeat (count form) [:cons])))

    (seq? form)
    (let [[fn-name & args] form]
      (prn form)
      (if (= fn-name 'quote)
        (do (prn args) (concat (mapcat #(compile-form vars fns %) (first args))
                               (repeat (dec (count form)) [:cons])))
        (cons [:ap (dec (count form))]
              ;; Push the args onto the stack in reverse order:
              ;; first arg is top of the stack
              (mapcat #(compile-form vars %) args))))

    :else
    (throw (IllegalArgumentException. (format "Don't know how to compile %s which is %s" form (type form))))))

(defn assign-addresses
  [fns]
  {:pre [(map? fns)
         (every? symbol? (keys fns))
         (every? map? (vals fns))]}
  (let [main (fns 'main)
        others (vals (sort (dissoc fns 'main)))]
    (loop [funcs (apply vector main others)
           index 0
           address 0]
      (if-let [func (get funcs index)]
        (let [new-funcs (update-in funcs [index] assoc :address address)
              address (+ address (count (:code func)))]
          (recur new-funcs (inc index) address))
        funcs))))

(defn code->str
  [fn-addrs line]
  {:pre [(map? fn-addrs)]
   :post [(string? %)]}
  (condp = (first line)
    :ldf
    (format "LDF %s ; load function %s"
            (fn-addrs (second line))
            (last line))

    (let [[op & args] line]
      (string/join " " (cons (string/upper-case (name op)) args)))))

(defn emit-code
  [fns]
  {:pre [(vector? fns)
         (every? map? fns)]
   :post [(string? %)]}
  (let [fn-addrs (into {} (map (juxt :name :address) fns))]
    (->> (mapcat :code fns)
         (map #(code->str fn-addrs %))
         (string/join "\n"))))

(defn compile-ai
  [file]
  {:pre [(string? file)]
   :post [(string? %)]}
  (let [prog (java.io.PushbackReader. (clojure.java.io/reader file))]
    (loop [form (read prog false nil)
           fns {}]
      (if form
        (let [fn-name (first form)
              args (second form)
              forms (drop 2 form)
              vars args
              fns (assoc fns fn-name {})
              code (mapcat #(compile-form vars fns %) forms)
              code (concat code [[:RTN]])]
          (recur (read prog false nil) (assoc fns fn-name {:name fn-name :code code})))
        (emit-code (assign-addresses fns))))))
