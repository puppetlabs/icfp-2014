(step [state world]
  '((inc state) right))

(main [init-world _]
  '(0 step))
