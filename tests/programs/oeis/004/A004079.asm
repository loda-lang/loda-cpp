; A004079: a(n) = maximal m such that an m X n Florentine rectangle exists.
; Submitted by LODA
; 1,2,2,4,4,6,6,7,8,10,10,12
; Formula: a(n) = ((gcd(-14,n-1)*(n-1))==(n-1))+n-1

#offset 1

sub $0,1
mov $1,-14
gcd $1,$0
mul $1,$0
equ $1,$0
add $0,$1
