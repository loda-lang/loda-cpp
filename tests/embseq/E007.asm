sub $0,1
mov $3,$0
mov $5,$0
add $5,1
lpb $5
  sub $5,1
  mov $2,5
  mov $0,$3
  sub $0,$5
  lpb $0
    add $0,2
    mov $1,1
    add $1,$0
    div $1,6
    sub $0,1
    mov $2,$0
    pow $2,2
    mov $0,$1
  lpe
  mov $0,$2
  sub $0,3
  add $4,$0
lpe
mov $0,$4
