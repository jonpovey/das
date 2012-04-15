; clangy stuff
:autoinit    ;;Init data stack register C
      SET C, SP
      SUB C, 256

:autostart
  JSR main
:autohalt SET PC, autohalt
    ; .file "/home/krasin/fib.c"
    ; .align  2
:fib
  SUB  C, 12 ; The Notch order
  SET  [10+C], X ; The Notch order
  SET  [8+C], 1 ; The Notch order
  SET  [6+C], 1 ; The Notch order
  SET  [4+C], 0 ; The Notch order
  SET  [0+C], X ; The Notch order
:.LBB0_1
  SET  J, [4+C] ; The Notch order
  SET  Z, [10+C] ; The Notch order
  SET  O, 65535 ; The Notch order, cmp Z, J, start
  IFE  J, Z ; The Notch order
  SET  O, 0 ; The Notch order
  IFG  J, Z ; The Notch order
  SET  O, 1 ; The Notch order, end
  IFN  O, 65535 ; The Notch order, jge
  SET  PC, .LBB0_4 ; The Notch order
  SET  PC, .LBB0_2 ; The Notch order
:.LBB0_2
  SET  J, [8+C] ; The Notch order
  SET  Z, [6+C] ; The Notch order
  ADD  J, Z ; The Notch order
  SET  [2+C], J ; The Notch order
  SET  J, [8+C] ; The Notch order
  SET  [6+C], J ; The Notch order
  SET  J, [2+C] ; The Notch order
  SET  [8+C], J ; The Notch order
  SET  J, [4+C] ; The Notch order
  ADD  J, 1 ; The Notch order
  SET  [4+C], J ; The Notch order
  SET  PC, .LBB0_1 ; The Notch order
:.LBB0_4
  SET  X, [8+C] ; The Notch order
  ADD  C, 12 ; The Notch order
  SET PC, POP ; The Notch order

  ; .align    2
:main
  SUB  C, 2 ; The Notch order
  SET  [0+C], 0 ; The Notch order
  SET  X, 5 ; The Notch order
  JSR  fib ; The Notch order
  ADD  C, 2 ; The Notch order
  SET PC, POP ; The Notch order