; A025810: Expansion of 1/((1-x^2)(1-x^5)(1-x^10)) in powers of x.
; 1,0,1,0,1,1,1,1,1,1,3,1,3,1,3,3,3,3,3,3,6,3,6,3,6,6,6,6,6,6,10,6,10,6,10,10,10,10,10,10,15,10,15,10,15,15,15,15,15,15,21,15,21,15,21,21,21,21,21,21,28,21,28,21,28,28,28,28,28,28,36,28,36,28,36,36,36,36,36,36

add $0,1
lpb $0
  mov $2,$0
  trn $2,1
  seq $2,8616 ; Expansion of 1/((1-x^2)(1-x^5)).
  trn $0,10
  add $1,$2
lpe
mov $0,$1
