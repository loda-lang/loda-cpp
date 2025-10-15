; Test that ROL in post-loop is rejected by IncrementalEvaluator
mov $1,$0
lpb $1
  add $3,$1
  sub $1,1
lpe
rol $2,3
mov $0,$3
