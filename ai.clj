(step [state world]
  '(state (% (nth (nth (nth (nth world 2) 0) 1) 0) 4)))

(asm "nth.gcc")
