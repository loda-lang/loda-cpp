; in
mov $1,5
lpb $0
  sub $0,1
  add $1,3
lpe
; out
mov $2,$0
max $2,0
mov $3,3
mul $3,$2
min $0,0
mov $1,5
add $1,$3
