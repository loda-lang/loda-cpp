; in
; Test that pow $0,2; nrt $0,2 is optimized to gcd $0,0
; because it computes absolute value for all inputs
pow $0,2
nrt $0,2
; out
gcd $0,0
