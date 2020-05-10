# .rep
  .rep a, 2
     mov r0, a*4
  .endr
  .rep a, 2
    .rep b, 2
      mov r0, a*4+b*5
    .endr
  .endr
