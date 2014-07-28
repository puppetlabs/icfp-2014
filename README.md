# icfp-2014 ICFP YEAHHHHHHHHHH

The (world's first?) LL Cool J compiler.

![LL Cool J approved](http://media.giphy.com/media/10DDXIuJMGzqbC/giphy.gif)

## Requirements

Our solution is written in Clojure, so you'll need a JVM and [leiningen](http://leiningen.org) to run it.

## Compiling the solutions

![LL Cool J approved](http://uproxx.files.wordpress.com/2013/02/ll-cool-j.gif)

**To compile `lambdaman.gcc`, our Lambda Man AI:**

`lein run *.llcoolj` (from this directory).

**To compile `ghost.ghc`, our Ghost AI:**

`lein run ghost.gfk` (from this directory).

## The LL Cool J Compiler

We determined that the optimal name for our LISP compiler is Lambda Lover Cool JoeBiden, or **LL Cool J** for short.
Although it isn't quite Clojure, the fact that it's implemented with the Clojure reader got us several neat features:

1. Free parsing. We could immediately start operating on code as data structures.
2. Several Clojure macros worked with minimal effort: `->`, `->>`, and `cond`.
3. `;` comments and reader-literal comments (`#_`) also worked automatically.

After a few hours of work, we had implemented `map` and `filter` (both non-lazy), but it took quite a bit longer to get
`let` and anonymous functions, which we really needed in order to write our AI effectively.

## The Lambda Man AI

It uses the A* search algorithm, is sensitive to panic mode, and starts by searching out power pills.

## The Ghostface Killah Assembler

We didn't write a full-featured language targeting GHC, instead opting to write mostly-pure assembly, with some
conveniences afforded by an assembler/linker, which we named Ghostface Killah. GFK gave us the following
features:

1. `#tags` to mark jump destinations, with `@tags` to refer to them.
2. `$vars`, each of which corresponds to a memory address.
3. Math macros like `ADD->` and `MUL->`, which don't mutate their mathematical operands, but instead store their
   results in a new address.
4. The `JMP` (unconditional jump) macro.
5. Game-specific convenience macros: `GHOST-POS`, `SQUARE-AT`, `LAMBDA-POS`, etc. 

## The Ghost AI

With the GFK assembler, we had enough to implement a basic AI with a few different modes:

1. Panic mode: overrides any other mode and does the opposite of chase mode (moving away from Lambda Man instead of towards him). It
   does not check for backtracking.
2. Chase mode: calculates the Manhattan distance to Lambda Man, then attempts to close the distance by moving along the axis (X or Y)
   where the distance is greatest. Moves are filtered through `dontbacktrack`, explained below.
3. Random mode: attempts to make use of whatever entropy it can get (including the ghost's own index and position) to choose an effectively
   random direction.

**`dontbacktrack`:** we found that it was pretty easy for a ghost to get stuck while attempting to chase Lambda Man, so we implemented a
simple check for loops. If a ghost's direction of travel changes from left->right->left, up->down->up, right->left->right, or down->up->down,
a `veto` operation will attempt to turn the ghost clockwise. This at least keeps the ghost from bouncing between two walls indefinitely.

Each ghost spends a different amount of time in chase mode before switching to random mode for 20 turns. The amount of time is entirely
dependent on the ghost's index, so that ghost 3 spends 80 turns in chase mode for every 20 in random mode.

![LL Cool J approved](http://24.media.tumblr.com/tumblr_lhv4e40Z1j1qgnq3do1_400.gif)
