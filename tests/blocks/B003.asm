; in
add $0,2
mov $1,1
lpb $0
  add $1,1
  trn $0,$1
lpe
mul $1,3
lpb $0
  add $1,1
  trn $0,$1
lpe
; out
; 1
mul $1,3
; 1
add $0,2
mov $1,1
; 2
lpb $0
  add $1,1
  trn $0,$1
lpe
