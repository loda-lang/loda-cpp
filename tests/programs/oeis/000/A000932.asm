; A000932: a(n) = a(n-1) + n*a(n-2); a(0) = a(1) = 1.
; 1,1,3,6,18,48,156,492,1740,6168,23568,91416,374232,1562640,6801888,30241488,139071696,653176992,3156467520,15566830368,78696180768,405599618496,2136915595392,11465706820800,62751681110208,349394351630208,1980938060495616,11414585554511232,66880851248388480,397903832329214208,2404329369780868608,14739348171986509056,91677888004974304512,578076377680529103360,3695124569849655456768,23927797788668174074368,156952282303255770518016,1042280800483978211269632,7006467528007697490954240

mov $1,1
lpb $0
  sub $0,1
  add $2,1
  mul $3,$2
  add $1,$3
  mul $3,-1
  add $3,$1
lpe
mov $0,$1