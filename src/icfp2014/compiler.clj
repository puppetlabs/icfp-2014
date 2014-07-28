(ns icfp2014.compiler
  (:require [clojure.java.io]
            [clojure.string :as string]
            [clojure.walk :as walk]
            [spyscope.core :as spy]))

(def globals (atom []))
(def ^:dynamic *scope* [])
(def ^:dynamic *cur-fun* "")

(def macros
  {'up    [[:ldc 0 "; up"]]
   'right [[:ldc 1 "; right"]]
   'down  [[:ldc 2 "; down"]]
   'left  [[:ldc 3 "; left"]]
   'true  [[:ldc 1 "; true"]]
   'false [[:ldc 0 "; false"]]
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
             (into [] (concat (apply concat nums) (repeat (dec (count nums)) [:add "; +"])))))
   '-   (fn
          ([x] (concat [[:ldc 0 "; -"]] x [[:sub "; -"]]))
          ([x y & nums]
             (into [] (concat
                       x y [[:sub "; -"]]
                       (apply concat (for [num nums]
                                       (concat num [[:sub "; -"]])))))))
   '*   (fn
          ([] [[:ldc 1 "; *"]])
          ([& nums]
             (into [] (concat (apply concat nums) (repeat (dec (count nums)) [:mul "; *"])))))
   '/   (fn
          ([x] (concat [[:ldc 1 "; /"]] x [[:div "; -"]]))
          ([x y] (concat x y [[:div "; /"]])))

   ;; cons ops
   'car (fn [cons]
          (concat cons [[:car]]))

   'cdr (fn [cons]
          (concat cons [[:cdr]]))

   'cons (fn [car cdr]
           (concat car cdr [[:cons]]))

   'atom? (fn [val]
            (concat val [[:atom]]))
   })

(defn fail-on-qualified-symbols
  [code]
  (let [fail-fn (fn [form]
                  (when (and (symbol? form)
                             (re-find #"/" (str form)))
                    (throw (IllegalArgumentException.
                             (format "Found qualified symbol %s" form)))))]
    (walk/postwalk fail-fn code)
    true))

(defn tag-with
  [label [stmt & stmts]]
  {:pre [(vector? stmt)
         (not (some vector? stmt))
         (every? vector? stmts)]
   :post [(vector? %)
          (every? vector? %)]}
  (let [labeled (conj stmt (format "; #%s" label))]
    (vec (concat [labeled] stmts))))

(defn store-local
  [name]
  (let [cur-scope (first *scope*)
        index (.indexOf cur-scope name)]
    (if-not (neg? index)
      [:st 0 index (format "; store var %s" name)]
      (throw (IllegalArgumentException. (format "Tried to store to unknown scope var: %s" name))))))

(defn load-local
  [name]
  (loop [scope-stack  *scope*
         depth 0]
    (let [cur-scope (first scope-stack)
          parents (next scope-stack)
          index (.indexOf cur-scope name)]
    (if-not (neg? index)
      [:ld depth index (format "; load var %s" name)]
      (if parents
        (recur parents (inc depth)))))))

(defn load-global
  [name]
  [:ld (count *scope*) (str "^" name) (format "; load var %s" name)])

(defn load-symbol
  [name]
  {:post [(vector? %)
          (not (some vector? %))]}
  (if-let [load-stmt (or (load-local name) (load-global name))]
    load-stmt
    (throw (IllegalArgumentException. (format "Could not find symbol %s" name)))))

;; Need to declare this early because compile-form and compile-function are mutually recursive
(declare compile-function)

(defn compile-form
  [fns form]
  {:pre [(not (nil? form))]
   :post [(vector? %)
          (every? vector? %)]}
  (vec
    (cond
      (integer? form)
      [[:ldc form]]

      (contains? macros form)
      (macros form)

      (symbol? form)
      [(load-symbol form)]

      (map? form)
      (if (empty? form)
        [[:ldc 0]]
        (concat (apply concat (for [[k v] form]
                                (concat (compile-form fns k) (compile-form fns v) [[:cons]])))
                [[:ldc 0]]
                (repeat (count form) [:cons])))

      (vector? form)
      (if (empty? form)
        [[:ldc 0]]
        (concat (mapcat #(compile-form fns %) form)
                [[:ldc 0]]
                (repeat (count form) [:cons])))

      (not (seq? form))
      (throw (IllegalArgumentException. (format "Don't know how to compile %s which is %s" form (type form))))

      (= (first form) 'if)
      (let [[_ pred then else] form
            fn-name (:name (last fns))
            pred-codes (compile-form fns pred)
            then-codes (if then
                         (compile-form fns then)
                         (throw (IllegalArgumentException. (format "Why have an if without a then? %s" form))))
            else-codes (if else
                         (compile-form fns else)
                         (compile-form fns 0))

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

      (= (first form) 'foreach)
      (let [[_ [x xs] body] form
            xs-sym (gensym (str *cur-fun* "-xs"))
            body-tag (gensym (format "%s-for-body" *cur-fun*))
            cond-tag (gensym (format "%s-for-cond" *cur-fun*))
            setup-tag (gensym (format "%s-for-setup" *cur-fun*))
            start-tag (gensym (format "%s-for-start" *cur-fun*))
            done-tag (gensym (format "%s-for-done" *cur-fun*))
            loop-tag (gensym (format "%s-for-loop" *cur-fun*))]
        (binding [*scope* (cons [x xs-sym] *scope*)]
          (vec
           (concat
            [[:ldc 0]
             [:tsel (str "@" setup-tag) (str "@" setup-tag)]]

            (tag-with body-tag [(load-local xs-sym)])
            [[:car]
             (store-local x)
             (load-local xs-sym)
             [:cdr]
             (store-local xs-sym)]
            (compile-form fns body)

            (tag-with cond-tag [(load-local xs-sym)])
            [[:atom]
             [:tsel (str "@" done-tag) (str "@" loop-tag)]]

            (tag-with loop-tag [[:ldc 0]])
            [[:sel (str "@" body-tag) (str "@" body-tag)]
             [:cons]
             [:join]]

            (tag-with done-tag [[:ldc 0] [:join]])

            (tag-with start-tag (compile-form fns xs))
            [(store-local xs-sym)
             [:ldc 0]
             [:sel (str "@" cond-tag) (str "@" cond-tag)]
             [:rtn]]

            (tag-with setup-tag
                      [[:dum 2]
                       [:ldc 0]
                       [:ldc 0]])
            [[:ldf (str "@" start-tag)]
             [:rap 2]]))))

      ; list declaration
      (= (first form) 'quote)
      (concat (mapcat #(compile-form fns %) (second form))
              (repeat (dec (count form)) [:cons]))

      ;; Variable declaration
      (= (first form) 'def)
      (let [[_ var-name val] form]
        (if (neg? (.indexOf @globals (name var-name)))
          (swap! globals conj (name var-name)))
        (concat
         (compile-form fns val)
         [[:st (count *scope*) (str "^" var-name) (format "; store %s" var-name)]]))

      (or (= (first form) 'lambda) (= (first form) 'fn*))
      (let [[_ args & forms] form
            fn-name (gensym (str (*cur-fun* "-lambda")))
            fn-end (str fn-name "-end")
            fn-form (cons fn-name (rest form))]
        (concat
         [[:ldc 0]
          [:tsel (str "@" fn-end) (str "@" fn-end)]]
         (:code (compile-function  fn-form  fns))
         (tag-with fn-end [[:ldf (str "@" fn-name)]])))

      ;; function call
      :else
      (let [[fn-form & args] form
            evaled-args (map #(compile-form fns %) args)]
        (if-let [builtin (builtins fn-form)]
          (apply builtin evaled-args)
          (concat
            ;; Push the args onto the stack
            (apply concat evaled-args)
            (compile-form fns fn-form)
            [[:ap (count args)]]))))))

(defn resolve-sym
  [name col]
  (let [idx (.indexOf col name)]
    (if-not (neg? idx)
      (str idx))))

(defn resolve-references
  [{:keys [lines fns]}]
  {:pre [(sequential? lines)
         (every? string? lines)]
   :post [(sequential? %)
          (every? string? %)]}
  (let [label-address (fn [index line]
                        (if-let [labels (map last (re-seq #"#(\S+)" line))]
                          (for [label labels]
                            [label (str index)])))
        labels (->> lines
                    (map-indexed label-address)
                    (apply concat)
                    (remove nil?)
                    (into {}))
        resolve-label #(get labels (last %) (last %))
        functions (map #(str (:name %)) (rest fns))
        resolve-global #(resolve-sym  (second %)  (concat functions @globals))]
    (for [line lines]
      (let [labels_resolved (clojure.string/replace line #"@(\S+)" resolve-label)
            fns_resolved (clojure.string/replace labels_resolved #"\^(\S+)" resolve-global)]
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
              [:ldc 0]
              [:cons]
              (load-global 'step)
              [:cons]
              [:rtn "; end main"]]
        main {:name 'main
              :code code
              :length (count code)}]
    (conj fns main)))

(defn generate-prelude
  [fns]
  (let [loads  (for [func fns]
                 [:ldf (format "@%s ; load %s" (:name func) (:name func))])
        global-loads (for [global @globals]
                       [:ldc 0 (format "; initialize %s" global)])
        frame-size (+ (count @globals) (count fns))
        code (concat [[:dum frame-size "; #prelude"]]
                     loads
                     global-loads
                     [[:ldf "@main ; load main"]
                      [:rap frame-size]
                      [:rtn "; end prelude"]])
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
  {:pre [(seq? code)
         (fail-on-qualified-symbols code)]
   :post [(vector? (:code %))
          (every? vector? (:code %))]}
  (binding [*scope* (cons args *scope*)
            *cur-fun* name]
    (let [code (->> body
                    (mapcat #(compile-form fns %))
                    (tag-with name))
          code (concat code [[:rtn (str "; end " name)]])]
      {:name name
       :code (vec code)
       :length (count code)})))

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
                     (compile-function (walk/macroexpand-all form) fns))]
          (recur (read prog false nil) (conj fns func)))
        (-> fns
            (generate-main)
            (generate-prelude)
            (emit-code)
            (resolve-references)
            (vec)
            (#(string/join "\n" (conj % nil))))))))
