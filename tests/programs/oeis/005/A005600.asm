; A005600: Decimal expansion of reciprocal of fine-structure constant alpha.
; 1,3,7,0,3,5,9,9,9

#offset 3

mov $1,$0
mod $1,4
equ $1,0
sub $0,3
min $0,6
mul $0,3
add $0,1
sub $0,$1
mod $0,10
