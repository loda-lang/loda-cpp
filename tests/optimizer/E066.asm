; in
; Test that pow $0,3; nrt $0,3 with odd exponent is NOT optimized
; because it would cause overflow for negative values
pow $0,3
nrt $0,3
add $0,1
; out
pow $0,3
nrt $0,3
add $0,1
