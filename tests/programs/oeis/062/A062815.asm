; A062815: a(n) = Sum_{i=1..n} i^(i+1).
; 1,9,90,1114,16739,296675,6061476,140279204,3627063605,103627063605,3242055440326,110235260819398,4047611646518687,159615707204330911,6728024062917221536,301875929242270047392,14364960381309995038401,722600305736647671396033,38312573763282605864751634,2135464573763282605864751634,124829791959868915554868364475,7636243093972699178281096283323,487887007090474675968446853226364,32497545651497293662746402201476988,2252943594901810374510009738383117613,162312052680287900455223541236788415789

add $0,1
lpb $0
  mov $2,$0
  pow $2,$0
  mul $2,$0
  sub $0,1
  add $1,$2
lpe
mov $0,$1
