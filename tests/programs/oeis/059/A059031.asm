; A059031: Fifth main diagonal of A059026: a(n) = B(n+4,n) = lcm(n+4,n)/(n+4) + lcm(n+4,n)/n - 1 for all n >= 1.
; 5,3,9,2,13,7,17,4,21,11,25,6,29,15,33,8,37,19,41,10,45,23,49,12,53,27,57,14,61,31,65,16,69,35,73,18,77,39,81,20,85,43,89,22,93,47,97,24,101,51,105,26,109,55,113,28,117,59,121,30,125,63,129,32,133,67

#offset 1

add $0,2
mov $1,2
add $1,$0
gcd $1,4
mul $0,2
div $0,$1
sub $0,1
