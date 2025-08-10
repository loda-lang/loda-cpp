sub $0,1
mov $7,$0
mov $5,$0
lpb $5
  sub $5,1
  mov $0,$7
  sub $0,$5
  mov $1,0 ; begin of virtual sequence 1 with input $0
  mov $2,2
  lpb $0
    mov $3,$0
    lpb $3
      mov $4,$0
      mod $4,$2
      neq $4,0
      sub $2,1
      sub $3,$4
    lpe
    div $0,$2
    lpb $0
      div $0,2
      add $1,$2
    lpe
  lpe ; end of virtual sequence 1 with output $1
  add $6,$1
lpe
mov $0,$6
