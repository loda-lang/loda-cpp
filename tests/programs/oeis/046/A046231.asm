; A046231: Numbers whose cube is palindromic in base 4.
; 0,1,5,17,65,257,1025,4097,16385,65537,262145,1048577,4194305,16777217,67108865,268435457,1073741825,4294967297

#offset 1

mov $2,1
sub $0,1
lpb $0
  sub $0,1
  add $1,$2
  mul $2,4
  mov $3,$1
  mov $1,1
lpe
mov $0,$3
