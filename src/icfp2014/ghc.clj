(ns icfp2014.ghc
  (:require [clojure.java.io :as io]
            [clojure.string :as s]))

(defn comment?
  "Return true if the line contains NO CODE."
  [line]
  (boolean (or (s/blank? line)
               (re-seq #"^\s*;.*" line))))

(defn hashtag
  "Return the name of a hashtag, if any. Otherwise return nil."
  [line]
  (last (first (re-seq #"#(\S+)" line))))

(defn vars
  "Return a vector of all vars on the line. If none, return an empty vector"
  [line]
  {:post [(vector? %)]}
  (mapv second (re-seq #"\$([\w-!?><\[\]]+)" line)))

(defn attag
  "Return the name of an @tag, if any. Otherwise return nil."
  [line]
  (last (first (re-seq #"@([a-zA-Z\-\[\]]+)" line))))

(defn scan-hashtag
  "Returns a vector of [hashtag line-number] if the line contains a hashtag.
  Otherwise returns nil."
  [index line]
  (when-let [tag (hashtag line)]
    [tag index]))

(defn replace-attag
  "Given a string (line) and a map of tags to line numbers, replace any instance
  of @tag with the corresponding number. Otherwise return the line."
  [tag-map line]
  (if-let [tag (attag line)]
    (let [num (or (tag-map tag)
                  (throw (IllegalArgumentException. (format "No target for @%s" tag))))
          processed-line (s/replace line (str "@" tag) (str num))]
      (format "%s ; @%s = %d" processed-line tag num))
    line))

(defn tag-lines
  [seq-of-lines]
  (let [source (->> seq-of-lines
                    (filter (complement comment?)))
        tag-map (->> source
                     (map-indexed scan-hashtag)
                     (into {}))]
    (map (partial replace-attag tag-map) source)))

(defn replace-vars
  "Given a string (line) and a map of tags to line numbers, replace any instance
  of @tag with the corresponding number. Otherwise return the line."
  [var-lst line]
  (reduce (fn [l var]
            (let [idx (.indexOf var-lst var)]
              (-> (s/replace l
                             (str "$" var)
                             (str "[" idx "]"))
                  (str (format " ; $%s = %d" var idx)))))
          line
          (vars line)))

(defn var-lines
  [seq-of-lines]
  (let [source  (->> seq-of-lines
                     (filter (complement comment?)))
        var-lst (vec (mapcat vars source))]
    (if (> (count var-lst) 255)
      (throw (IllegalArgumentException. "The number of variables is too damn high!")))
    (mapv (partial replace-vars var-lst) source)))

(defn check-length
  "The maximum length for a GHC program is 256 instructions.
  Throw an exception if the program is too long, otherwise
  just return it."
  [lines]
  (if (> (count lines) 256)
    (throw (IllegalArgumentException. (format "This program is too damn long! %d lines!" (count lines))))
    lines))

(defn macrolize-mathop
  [op args]
  (let [[dest,x,y] args]
    [(format "MOV $TMP,%s" x)
     (format "%s $TMP,%s" op y)
     (format "MOV %s,$TMP" dest)]))

(defn macrolize
  [line]
  (let [parts  (s/split line #"\s")
        op     (first parts)
        args   (s/split (second parts) #",")
        extra  (str " " (s/join " " (drop 2 parts)))
        instrs (case op

                 "SUB->"
                 (macrolize-mathop "SUB" args)

                 "ADD->"
                 (macrolize-mathop "ADD" args)

                 "MUL->"
                 (macrolize-mathop "MUL" args)

                 "DIV->"
                 (macrolize-mathop "DIV" args)

                 "AND->"
                 (macrolize-mathop "AND" args)

                 "OR->"
                 (macrolize-mathop "OR" args)

                 "XOR->"
                 (macrolize-mathop "XOR" args)

                 "JMP"
                 (let [[dest] args]
                   [(format "JEQ %s,0,0" dest)]))

        instrs (update-in instrs [0] str extra)]
         (s/join "\n" instrs)))

(defn tag-file*
  [fin-path]
  (->> fin-path
       io/reader
       line-seq
       tag-lines
       (map macrolize)
       var-lines
       check-length
       (s/join "\n")))

(defn tag-file
  [fin-path fout-path]
  (->> fin-path
       tag-file*
       (spit fout-path)))


;; TODO:
;;
;; * "prelude/coda/appendix" of stdlib "funcs"
