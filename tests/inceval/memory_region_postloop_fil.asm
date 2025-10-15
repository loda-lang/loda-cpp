; Test that FIL in post-loop is rejected by IncrementalEvaluator
mov $1,$0
lpb $1
  add $3,$1
  sub $1,1
lpe
fil $2,3
mov $0,$3
