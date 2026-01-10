; A005310: Fermionic string states.
; 2,2,6,14,30,62,126,246,472

mov $3,2
lpb $0
  sub $0,1
  sub $3,$4
  add $5,$3
  add $2,$4
  mov $4,$2
  div $1,4
  add $2,2
  add $2,$1
  mov $3,1
  mov $1,$5
lpe
mov $0,$4
mul $0,2
add $0,2
