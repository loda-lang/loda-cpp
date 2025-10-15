; Test that FIL in pre-loop is rejected by IncrementalEvaluator
mov $1,$0
fil $2,3
lpb $1
  add $3,$1
  sub $1,1
lpe
mov $0,$3
