#offset 1

mov $1,$0
pow $1,10
lpb $1
  mov $5,$3 ; begin of embedded sequence 1 with input $3
  max $5,1
  log $5,10
  add $5,1
  mov $6,10
  pow $6,$5
  mov $9,0
  mov $10,1
  mov $13,0
  mov $4,$3
  lpb $4
    mul $13,$10
    mul $13,2
    mov $14,$9
    pow $14,2
    mov $7,$10
    pow $7,2
    sub $13,$14
    add $14,$7
    mov $7,$14
    sub $7,$13
    mov $11,$4
    max $11,1
    log $11,2
    mov $12,2
    pow $12,$11
    ban $12,$3
    neq $12,0
    div $4,2
    mul $7,$12
    mov $8,$13
    mul $8,$12
    add $13,$7
    add $14,$8
    mov $9,$13
    mod $9,$6
    mov $10,$14
    mod $10,$6
  lpe
  mov $4,$9
  equ $4,$3 ; end of embedded sequence 1 with output $4
  add $2,$4
  mov $4,$2
  geq $4,$0
  bxo $4,1
  mul $1,$4
  sub $1,1
  add $3,1
lpe
mov $0,$3
