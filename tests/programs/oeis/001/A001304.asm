; A001304: Expansion of 1/((1-x)^2*(1-x^2)*(1-x^5)).
; 1,2,4,6,9,13,18,24,31,39,49,60,73,87,103,121,141,163,187,213,242,273,307,343,382,424,469,517,568,622,680,741,806,874,946,1022,1102,1186,1274,1366,1463,1564,1670,1780,1895,2015,2140,2270,2405,2545,2691,2842,2999,3161,3329,3503,3683,3869,4061,4259,4464,4675,4893,5117,5348,5586,5831,6083,6342,6608,6882,7163,7452,7748,8052,8364,8684,9012,9348,9692,10045,10406,10776,11154,11541,11937,12342,12756,13179,13611,14053,14504,14965,15435,15915,16405,16905,17415,17935,18465

lpb $0
  mov $2,$0
  sub $0,1
  seq $2,115 ; Denumerants: Expansion of 1/((1-x)*(1-x^2)*(1-x^5)).
  add $1,$2
lpe
add $1,1
mov $0,$1