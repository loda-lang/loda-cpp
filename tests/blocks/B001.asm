; in
; this is a comment that should be ignored
mov $1,$0
mov $2,$0 ; another comment to be ignored
mov $0,1
lpb $2
  sub $1,$0
  add $0,$1
  sub $1,$0
  sub $2,1
lpe
mov $1,$0
; out
; 1
mov $1,$0
; 1
mov $1,$0
mov $2,$0
mov $0,1
; 1
lpb $2
  sub $1,$0
  add $0,$1
  sub $1,$0
  sub $2,1
lpe
