(step [state world]
  [state 1])

(main [init-world _]
  [0 step])

[0 1 2 3]

(0 (1 (2 3)))

LDC 3
LDC 2
CONS
LDC 1
CONS
LDC 0
CONS

LDC 0
LDC 1
LDC 2
LDC 3
CONS
CONS
CONS

(0 (1 (3 2)))
