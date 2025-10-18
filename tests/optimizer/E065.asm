; in
; Test that pow $0,2; nrt $0,2 is NOT optimized away
; because it changes negative values to positive
mov $0,5
sub $0,10
pow $0,2
nrt $0,2
; out
mov $0,5
sub $0,10
pow $0,2
nrt $0,2
