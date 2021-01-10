; in
mov $1,3
lpb $0,2
  lpb $0,2
    add $1,5
  lpe
lpe
mov $2,4
lpb $0,2
  lpb $0,3
    add $1,3
  lpe
lpe
; out
mov $1,3
lpb $0,2
  add $1,5
lpe
mov $2,4
lpb $0,2
  lpb $0,3
    add $1,3
  lpe
lpe
