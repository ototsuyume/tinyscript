count=0;
function printdata(n)
{
    i=0;
    while(i<8)
	{
	    print('+'*n[i]+'Q'+'+'*(7-n[i]));
	    i=i+1;
	}
    print('-----------------------------');
}

function check(n,cur)
{
    i=0;
	while(i<cur)
	{
		
    	if(n[i]==n[cur]||(abs(i-cur)==abs(n[i]-n[cur]))) return false;
	    i=i+1;
	}
	return true;
}

function queen(n,arr)
{
	global count;
	i=0;
	while(i<8)
	{
		
		arr[n]=i;
		if(check(arr,n))
	    {
		    if(n>=7){printdata(arr);count=count+1;}else{queen(n+1,arr);}
		}
	    i=i+1;
	}
}
		queen(0,[0,0,0,0,0,0,0,0]);
		print(count);


