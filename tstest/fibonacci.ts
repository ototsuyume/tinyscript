
function fib(a,b,n)
{	
	c = a+b;
	if(!n)
		return c;
	else
		return fib(b,c,n-1);
}


function fibonacci(n)
{
	if(n==1||n==2)
		return 1;
	return fib(1,1,n-3);
}


print(fibonacci(3));
print(fibonacci(4));
print(fibonacci(5));
print(fibonacci(10));