; in
mov $1,$0
add $1,2
bin $1,$0
mov $3,5
prg $1,4222124650659841 ; TODO: support parsing "P000001"
add $1,$2
add $1,$3
mov $0,$1
; out
mov $1,$0
add $1,2
bin $1,$0
mov $3,5
mov $2,0
lpb $1
  add $2,1
  sub $1,$2
lpe
add $1,$2
add $1,$3
mov $0,$1
