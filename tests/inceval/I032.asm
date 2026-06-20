mov $1,2
fil $1,4
add $4,1
fil $4,3
clr $2,3
lpb $0
  sub $0,1
  add $1,$3
  add $3,$5
  add $4,$6
  add $1,$4
  add $1,$5
  add $5,$4
  add $4,$3
  add $3,$6
lpe
mov $0,$1
