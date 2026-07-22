; in
mul $0,2
seq $0,0
add $0,4
; out
mul $0,2
mov $1,1
lpb $0
  mul $1,$0
  sub $0,1
lpe
mov $0,$1
add $0,4
