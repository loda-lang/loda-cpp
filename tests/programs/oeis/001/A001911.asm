; A001911: a(n) = Fibonacci(n+3) - 2.
; 0,1,3,6,11,19,32,53,87,142,231,375,608,985,1595,2582,4179,6763,10944,17709,28655,46366,75023,121391,196416,317809,514227,832038,1346267,2178307,3524576,5702885,9227463,14930350,24157815,39088167,63245984,102334153,165580139,267914294,433494435,701408731,1134903168,1836311901,2971215071,4807526974,7778742047,12586269023,20365011072,32951280097,53316291171,86267571270,139583862443,225851433715,365435296160,591286729877,956722026039,1548008755918,2504730781959,4052739537879,6557470319840

mov $1,2
mov $2,1
lpb $0
  sub $0,1
  mov $3,$2
  mov $2,$1
  add $1,$3
lpe
sub $1,2
mov $0,$1