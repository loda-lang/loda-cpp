a(n) = if(n==3,-2,if(n==2,2,if(n==1,-1,if(n==0,1,local(l1=a(n-1)); (n-1)*(2*a(n-2)+l1)-l1))))
for(n=0,11,print(a(n)))
quit
