; in
mov $1,$0
mov $2,4
prg $1,4222124650659842 ; TODO: support parsing "P000002"
add $1,$2
; out
mov $1,$0
mov $2,4
mov $3,$1
add $3,2
bin $3,$1
mov $1,$3
add $1,$2
