; A001401: Number of partitions of n into at most 5 parts.
; 1,1,2,3,5,7,10,13,18,23,30,37,47,57,70,84,101,119,141,164,192,221,255,291,333,377,427,480,540,603,674,748,831,918,1014,1115,1226,1342,1469,1602,1747,1898,2062,2233,2418,2611,2818,3034,3266,3507,3765,4033,4319,4616,4932,5260,5608,5969,6351,6747,7166,7599,8056,8529,9027,9542,10083,10642,11229,11835,12470,13125,13811,14518,15257,16019,16814,17633,18487,19366,20282,21224,22204,23212,24260,25337,26455,27604,28796,30020,31289,32591,33940,35324,36756,38225,39744,41301,42910,44559

mov $3,3
add $0,3
lpb $0
  sub $0,$3
  mov $2,$0
  add $2,3
  mov $4,$2
  pow $4,2
  mul $4,3
  sub $2,2
  pow $2,3
  div $2,3
  mul $2,2
  add $2,$4
  add $2,24
  div $2,48
  add $1,$2
  mov $3,10
lpe
mov $0,$1