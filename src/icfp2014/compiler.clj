(ns icfp2014.compiler
  (:require [clojure.java.io]
            [clojure.string :as string]
            [clojure.walk :as walk]
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
          (concat
            x
            [[:ldc 1 "; inc"]
             [:add "; inc"]]))
   'dec (fn [x]
          (concat
            x
            [[:ldc 1 "; dec"]
             [:sub "; dec"]]))

   '+   (fn
          ([] [[:ldc 0 "; +"]])
          ([& nums]
             (into [] (concat nums (repeat (dec (count nums)) [:add "; +"])))))
   '-   (fn
          ([x] (concat [[:ldc 0 "; -"]] x [[:sub "; -"]]))
          ([x y & nums]
             (into [] (concat
                       x y [[:sub "; -"]]
                       (apply concat (for [num nums]
                                       [num [:sub "; -"]]))))))
   '*   (fn
          ([] [[:ldc 1 "; *"]])
          ([& nums]
             (into [] (concat nums (repeat (dec (count nums)) [:mul "; *"])))))
   '/   (fn
          ([x] (concat [[:ldc 1 "; /"]] x [[:div "; -"]]))
          ([x y] (concat x y [[:div "; /"]])))

   ;; Comparison ops

   '=   (fn [x y] (concat x y [[:ceq "; ="]]))
   '>   (fn [x y] (concat x y [[:cgt "; >"]]))
   '>=  (fn [x y] (concat x y [[:cgte "; >="]]))
   '<   (fn [x y]
          ;; if x isn't >= y, then x < y
          (concat
            x
            y
            [[:cgte "; <"]
             [:ldc 0 "; <"]
             [:ceq "; <"]]))
   '<=  (fn [x y]
          ;; if x isn't > y, then x <= y
          (concat
            x
            y
            [[:cgt "; <="]
             [:ldc 0 "; <="]
             [:ceq "; <="]]))

   ;; cons ops
   'car (fn [cons]
          (concat cons [[:car]]))

   'cdr (fn [cons]
          (concat cons [[:cdr]]))

   'cons (fn [car cdr]
           (concat car cdr [[:cons]]))

   })

(defn tag-with
  [label [stmt & stmts]]
  {:pre [(vector? stmt)
         (not (some vector? stmt))
         (every? vector? stmts)]
   :post [(vector? %)
          (every? vector? %)]}
  (let [labeled (conj stmt (format "; #%s" label))]
    (vec (concat [labeled] stmts))))

(defn load-var
  [vars name]
  (let [index (.indexOf vars name)]
    (if-not (neg? index)
      [:ld 0 index (format "; load var %s" name)])))

(defn load-fn
  ([name]
   (load-fn 1 name))
  ([frame name]
   [:ld frame (format "^%s" name) (format "; load fn %s" name)]))

(defn load-symbol
  [fns vars name]
  {:post [(vector? %)
          (not (some vector? %))]}
  (if-let [load-stmt (or (load-var vars name) (load-fn name))]
    load-stmt
    (throw (IllegalArgumentException. (format "Could not find symbol %s" name)))))

(defn compile-form
  [vars fns form]
  {:pre [form]
   :post [(vector? %)
          (every? vector? %)]}
  (vec
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

      (not (seq? form))
      (throw (IllegalArgumentException. (format "Don't know how to compile %s which is %s" form (type form))))

      (= (first form) 'if)
      (let [[_ pred then else] form
            fn-name (:name (last fns))
            pred-codes (compile-form vars fns pred)
            then-codes (compile-form vars fns then)
            else-codes (if else
                         (compile-form vars fns else)
                         (compile-form vars fns 0))

            pred-label (gensym (str fn-name "-pred"))
            then-label (gensym (str fn-name "-then"))
            else-label (gensym (str fn-name "-else"))]
        (concat [[:ldc 0]
                 [:tsel (str "@" pred-label) (str "@" pred-label)]]
                (tag-with then-label then-codes)
                [[:join]]
                (tag-with else-label else-codes)
                [[:join]]
                (tag-with pred-label pred-codes)
                [[:sel (str "@" then-label) (str "@" else-label)]]))

      (= (first form) 'cond)
      (compile-form vars fns (walk/macroexpand-all form))

      ;; list declaration
      (= (first form) 'quote)
      (concat (mapcat #(compile-form vars fns %) (second form))
              (repeat (dec (count form)) [:cons]))

      ;; function call
      :else
      (let [[fn-name & args] form
            evaled-args (map #(compile-form vars fns %) args)]
        (if-let [builtin (builtins fn-name)]
          (apply builtin evaled-args)
          (concat
            ;; Push the args onto the stack
            (apply concat evaled-args)
            [(load-fn fn-name)
             [:ap (count args)]]))))))

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
              (load-fn 0 'step)
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
  {:pre [(list? code)]
   :post [(vector? (:code %))
          (every? vector? (:code %))]}
  (let [fns (conj fns {:name name})
        code (->> body
                  (mapcat #(compile-form args fns %))
                  (tag-with name))
        code (concat code [[:rtn]])]
    {:name name
     :code (vec code)
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
