; 1,1,2,2,4,5,8,10
mov $1,1
lpb $0
  max $0,3
  sub $0,2
  add $1,$0
lpe
mov $0,$1
