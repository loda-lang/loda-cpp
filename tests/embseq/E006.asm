mul $0,2
mov $1,$0
pow $1,3
lpb $1
  mov $2,2
  sub $2,$0
  lpb $2
    mov $2,0
    mov $1,0
  lpe
  sub $1,1
  mov $2,$0
  mod $2,2
  add $3,1
  mov $4,180
  mul $4,$2
  add $0,$4
  div $0,2
lpe
mov $0,$3
sub $0,1
