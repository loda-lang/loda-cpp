; Test: collapseBasicLoop with region operation (fil)
; This test ensures that temporary cells are allocated correctly
; when region operations like fil are present
; in
fil $10,5
lpb $1,1
  sub $1,1
  add $2,$3
lpe
; out
fil $10,5
mov $15,$1
max $15,0
mov $16,$3
mul $16,$15
min $1,0
add $2,$16
