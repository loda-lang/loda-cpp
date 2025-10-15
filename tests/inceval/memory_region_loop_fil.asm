; Test that FIL in loop body is rejected by IncrementalEvaluator
mov $1,$0
lpb $1
  fil $2,3
  add $3,$1
  sub $1,1
lpe
mov $0,$3
