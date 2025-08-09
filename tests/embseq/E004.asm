#offset 3

sub $0,3
lpb $0
  mov $1,$0 ; begin of embedded sequence 1 with input $0
  div $1,2
  mul $1,2
  lpb $1
    bin $1,4
  lpe
  add $1,3 ; end of embedded sequence 1 with output $1
  mul $0,2
  pow $0,2
  mod $0,7
lpe
mov $0,$1
div $0,3
add $0,1
