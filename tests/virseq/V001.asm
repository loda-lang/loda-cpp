sub $0,2
mov $1,5
mov $4,$0
mul $4,2
add $4,3
mov $7,2
mov $9,$0
pow $9,5
lpb $9
  mov $2,5 ; begin of virtual sequence 1 with input $6
  mov $5,2
  mov $8,$6
  nrt $8,2
  div $8,3
  lpb $8
    mov $3,$6
    mod $3,$2
    neq $3,0
    add $2,$5
    mul $5,2
    mod $5,6
    sub $8,$3
  lpe
  equ $8,0 ; end of virtual sequence 1 with output $8
  sub $0,$8
  add $1,$7
  mov $6,$1
  mul $7,2
  mod $7,6
  sub $9,$0
lpe
mov $0,$6
max $0,$4
max $0,2
