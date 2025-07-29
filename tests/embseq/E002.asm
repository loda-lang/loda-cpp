mov $2,$0
pow $2,2
lpb $2
  mov $5,0 ; begin of embedded sequence 1 with input $1
  mov $7,3
  mov $8,0
  mov $3,$1
  add $3,3
  lpb $3
    sub $3,$7
    mov $6,$3
    max $6,0
    seq $6,161
    add $5,$6
    mov $7,1
    add $7,$8
    mul $7,7
    add $8,2
  lpe
  mov $3,$5
  min $3,1 ; end of embedded sequence 1 with output $3
  sub $0,$3
  add $1,1
  mov $4,$0
  max $4,0
  equ $4,$0
  mul $2,$4
  sub $2,1
lpe
mov $0,$1
