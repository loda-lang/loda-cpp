; A001147: Double factorial of odd numbers: a(n) = (2*n-1)!! = 1*3*5*...*(2*n-1).
; 1,1,3,15,105,945,10395,135135,2027025,34459425,654729075,13749310575,316234143225,7905853580625,213458046676875,6190283353629375,191898783962510625,6332659870762850625,221643095476699771875,8200794532637891559375,319830986772877770815625,13113070457687988603440625,563862029680583509947946875,25373791335626257947657609375,1192568192774434123539907640625,58435841445947272053455474390625,2980227913743310874726229193921875,157952079428395476360490147277859375,8687364368561751199826958100282265625

mov $1,1
mul $0,2
add $0,1
lpb $0
  sub $0,2
  mul $1,$0
lpe
mov $0,$1
