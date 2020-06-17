  .rep x, 2       #O
    .if 1         #O
        .rep y, 2 #O
            nop;  #O
        .endr     #O
    .endif        #O
  .endr           #O
  .if 0           #W:P145.20 unterminated .if
