(ns icfp2014.compiler
  (:require [clojure.java.io]
            [clojure.string :as string]
            [spyscope.core :as spy]))

(def macros
  {'up    [[:ldc 0 "; up"]]
   'right [[:ldc 1 "; right"]]
   'down  [[:ldc 2 "; down"]]
   'left  [[:ldc 3 "; left"]]
   })

(def builtins
  {
   ;; Primitive math

   'inc (fn [x]
          [x
           [:ldc 1 "; inc"]
           [:add "; inc"]])
   'dec (fn [x]
          [x
           [:ldc 1 "; dec"]
           [:sub "; dec"]])

   '+   (fn
          ([] [[:ldc 0 "; +"]])
          ([& nums]
             (into [] (concat nums (repeat (dec (count nums)) [:add "; +"])))))
   '-   (fn
          ([x] [[:ldc 0 "; -"] x [:sub "; -"]])
          ([x y & nums]
             (into [] (concat
                       [x y [:sub "; -"]]
                       (apply concat (for [num nums]
                                       [num [:sub "; -"]]))))))
   '*   (fn
          ([] [[:ldc 1 "; *"]])
          ([& nums]
             (into [] (concat nums (repeat (dec (count nums)) [:mul "; *"])))))
   '/   (fn
          ([x] [[:ldc 1 "; /"] x [:div "; -"]])
          ([x y] [x y [:div "; /"]]))

   ;; Comparison ops

   '=   (fn [x y] [x y [:ceq "; ="]])
   '>   (fn [x y] [x y [:cgt "; >"]])
   '>=  (fn [x y] [x y [:cgte "; >="]])
   '<   (fn [x y]
          ;; if x isn't >= y, then x < y
          [x
           y
           [:cgte "; <"]
           [:ldc 0 "; <"]
           [:ceq "; <"]])
   '<=  (fn [x y]
          ;; if x isn't > y, then x <= y
          [x
           y
           [:cgt "; <="]
           [:ldc 0 "; <="]
           [:ceq "; <="]])

   ;; cons ops
   'car (fn [cons]
          (concat cons [[:car]]))

   'cdr (fn [cons]
          (concat cons [[:cdr]]))

   'cons (fn [car cdr]
           (concat car cdr [[:cons]]))

   })

(defn load-var
  [vars name]
  (let [index (.indexOf vars name)]
    (if-not (neg? index)
      [:ld 0 index (format "; load var %s" name)])))

(defn load-fn
  ([fns name]
   (load-fn 1 fns name))
  ([frame fns name]
   (if-let [func (first (filter #(= (:name %) name) fns))]
     [:ld frame (format "^%s" name) (format "; load fn %s" name)])))

(defn load-symbol
  [fns vars name]
  (if-let [load-stmt (or (load-fn fns name) (load-var vars name))]
    load-stmt
    (throw (IllegalArgumentException. (format "Could not find symbol %s" name)))))

(defn compile-form
  [vars fns form]
  {:post [(sequential? %)
          (every? vector? %)]}
  (cond
    (integer? form)
    [[:ldc form]]

    (contains? macros form)
    (macros form)

    (symbol? form)
    [(load-symbol fns vars form)]

    (vector? form)
    (if (empty? form)
      [[:ldc 0]]
      (concat (mapcat #(compile-form vars fns %) form)
              [[:ldc 0]]
              (repeat (count form) [:cons])))

    (seq? form)
    (let [[fn-name & args] form]
      (if (= fn-name 'quote)
        (concat (mapcat #(compile-form vars fns %) (first args))
                (repeat (dec (count form)) [:cons]))
        (let [evaled-args (map #(compile-form vars fns %) args)]
          (if (builtins fn-name)
            (apply (builtins fn-name) evaled-args)
            (concat
              ;; Push the args onto the stack
              (apply concat evaled-args)
              [(load-fn fns fn-name)
               [:ap (count args)]])))))

    :else
    (throw (IllegalArgumentException. (format "Don't know how to compile %s which is %s" form (type form))))))

(defn resolve-references
  [{:keys [lines fns]}]
  {:pre [(sequential? lines)
         (every? string? lines)]
   :post [(sequential? %)
          (every? string? %)]}
  (let [label-address (fn [index line]
                        (if-let [label (last (first (re-seq #"#(\S+)" line)))]
                          [label (str index)]))
        labels (->> lines
                    (map-indexed label-address)
                    (remove nil?)
                    (into {}))
        resolve-label #(get labels (last %) (last %))
        functions (map #(str (:name %)) (rest fns))
        resolve-call #(str (.indexOf functions (last %)))]
    (for [line lines]
      (let [labels_resolved (clojure.string/replace line #"@(\S+)" resolve-label)
            fns_resolved (clojure.string/replace labels_resolved #"\^(\S+)" resolve-call)]
        fns_resolved))))

(defn code->str
  [line]
  {:post [(string? %)]}
  (if (string? line)
    line
    (let [[op & args] line]
      (string/join " " (cons (string/upper-case (name op)) args)))))

(defn generate-main
  [fns]
  ;; This depends on the initial state we want
  (let [code [[:ldc 0 "; #main"]
              (load-fn 0 fns 'step)
              [:cons]
              [:rtn]]
        main {:name 'main
              :code code
              :length (count code)}]
    (conj fns main)))

(defn generate-prelude
  [fns]
  (let [loads  (for [func fns]
                 [:ldf (format "@%s ; load %s" (:name func) (:name func))])
        code (concat [[:dum (count fns) "; #prelude"]]
                     loads
                     [[:ldf "@main ; load main"]
                      [:rap (count fns)]
                      [:rtn]])
        prelude {:name 'prelude
              :code code
              :length (count code)}]
    (vec (cons prelude fns))))

(defn emit-code
  [fns]
  {:pre [(sequential? fns)
         (every? map? fns)]
   :post [(every? string? (:lines %))]}
  (let [fn-addrs (into {} (map (juxt :name :address) fns))]
    {:lines (->> (mapcat :code fns)
                 (map code->str))
     :fns fns}))

(defn import-asm
  [fn-name file]
  (let [code (vec (line-seq (clojure.java.io/reader file)))]
    {:name fn-name
     :code code
     :length (count code)}))

(defn compile-function
  [[name args & body :as code] fns]
  {:pre [(list? code)]}
  (let [fns (conj fns {:name name})
        [stmt & stmts] (mapcat #(compile-form args fns %) body)
        code (concat [(conj stmt (format "; #%s" name))]
                     stmts
                     [[:rtn]])]
    {:name name
     :code code
     :length (count code)}))

(defn compile-ai
  [file]
  {:pre [(string? file)]
   :post [(string? %)]}
  (let [prog (java.io.PushbackReader. (clojure.java.io/reader file))]
    (loop [form (read prog false nil)
           fns []]
      (if form
        (let [func (if (= (first form) 'asm)
                     (import-asm (second form) (last form))
                     (compile-function form fns))]
          (recur (read prog false nil) (conj fns func)))
        (-> fns
            (generate-main)
            (generate-prelude)
            (emit-code)
            (resolve-references)
            (#(string/join "\n" %)))))))
