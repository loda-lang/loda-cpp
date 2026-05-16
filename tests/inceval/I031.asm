; 0,1,3,6,10,15,21,28
; Two independent ADD accumulators (previously rejected: stateful_cells.size() > 1)
lpb $0
  add $1,$0
  sub $0,1
  add $2,1
lpe
mov $0,$1
