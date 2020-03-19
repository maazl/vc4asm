add r0, r4, ra1.unpack8b
add.unpack8af r0, r4, ra1
add r0, r4.8a, ra1
# The next test is rather creepy. The 1st mov chooses SMI -13, while the 2nd mov identifies that 12 = ~-13 which requires an ALU swap. Then the result is "not rb0, 12;  v8min ra0.8abcdi, -13, -13".
# The disassembler converts this to "mov rb0, 12;  mov ra0.8abcdi, -13". If this is assembled again, the 2nd mov chooses "not ra0.8abcdi, -13" which again requires an ALU swap.
mov ra0, 0xf3f3f3f3; mov rb0, 12
mov rb0, 12;  mov ra0.8abcdi, -13
mov ra10.8bsi, 0
mov r0, 0x01010101
mov r0, 0x02020202
mov r0, 0x80808080
