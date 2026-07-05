; in
add $0,1
mov $1,$0
seq $1,123
add $0,1
bor $0,$1
; out
add $0,1
mov $1,$0
mov $2,1
lpb $1
  mul $2,$1
  sub $1,1
lpe
mov $1,$2
add $0,1
bor $0,$1
