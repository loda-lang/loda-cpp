; Test: mergeRepeated with region operation (clr)
; This test ensures that temporary cells are allocated correctly
; when region operations are present
; in
clr $5,10
add $0,$1
add $0,$1
add $0,$1
; out
clr $5,10
mov $15,$1
mul $15,3
add $0,$15
