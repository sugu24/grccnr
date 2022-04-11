#!/bin/bash
assert(){
	expected="$1"
	input="$2"

	./grccnr "$input" > temp.s
	cc -o temp temp.s link.o
	./temp
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$input => $expected expected, but got $actual"
		exit 1
	fi
}

assert 3 'int main() { int a = 1 + 1; a = a + 1; return a; }'

assert 1 'int main() { char x[3]; x[0] = 1; return x[0]; }'
assert 1 'int main() { char x[3]; x[0] = 1; int y = 1; return x[0]; }'
assert 3 'int main() { char x[3]; x[0] = -1; int y = 4; return x[0] + y; }'
assert 3 'int main() { char x[3]; x[1] = -1; x[0] = x[1]; int y = 4; return x[0] + y; }'
assert 3 'int main() { char x[3]; *(x+1) = -1; x[0] = x[1]; int y = 4; return x[0] + y; }'

assert 0  'int x; int y[20]; int main() { return x + y[5]; + y[0]; } '
assert 1 'int main() { int a[3]; int two = 2; a[two] = 1; int y = a[two]; return y; }'
assert 3 'int main() { int a[2]; *a = 1; *(a+1) = 2; int *p; p = a; return *p+*(p+1); }'
assert 2 'int main() { int a[2]; *(a+1) = 2; return *(a+1); }'
assert 2 'int main() { int *a; int y = 2; a = &y; return *a; }'
assert 3 'int main() { int x; int *y; y = &x; *y = 3; return x;}'
assert 1 'int main() { int a[10]; a[0] = 1; return a[0]; }'
assert 4 'int main() { int a = 1; int b = 2; int c = 3; int *d = &c; int **e = &d; *(*e+1) = 4; return b;}'


assert 4 'int main() { int x; int *y; return sizeof(x); }'
assert 8 'int main() { int x; int *y; return sizeof(y); }'
assert 4 'int main() { int x; int *y; return sizeof(x+3); }'
assert 8 'int main() { int x; int *y; return sizeof(y+3); }'
assert 4 'int main() { int x; int *y; return sizeof(*y); }'
assert 4 'int main() { int x; int *y; return sizeof(1); }'
assert 4 'int main() { int x; int *y; return sizeof(sizeof(1)); }'

assert 11 'int foo() { return 11; } int bar(int a,int b) { return a * 10 + b; } int main() { int c = bar(1,1); return c; }'
assert 11 'int bar(int a,int b) { return a * 10 + b; } int main() { int c = bar(1,1); return c; }'
assert 4 'int main() { int *a; int b = 4; a = &b; *a = *a - 1; *a = *a - 1; return 2*b;}'
assert 2 'int main() { int a = 1; int b = 1; return b + 1; }'


assert 4 'int bar(int *x) { *x = 4; } int main() { int *a; int b = 1; a = &b; bar(a); return *a; }'
assert 10 'int bar(int **x) { int y = 10; *x = &y; } int main() { int *a; int b; b = 1; a = &b; bar(&a); return *a; }'

assert 3 'int main() { int x; x = 3; int *y; y = &x; return *y; }'
assert 3 'int main() { int x = 3; int y = 5; int *z = &y + 4; return *z; }'

assert 20 'int main() { int bar = 2; if (bar == 2) bar = bar * 10; return bar; }'
assert 1 'int main() { int bar = 0; if (bar == 0) bar = 1; else if (bar > 0) bar = bar * 10; return bar; }'
assert 10 'int main() { int bar = 0; if (bar == 1) bar = bar; else if (bar == 2) bar = bar; else bar = 10; return bar; }'
assert 1 'int main() { int bar = 1; if (bar == 1) bar = bar; else if (bar == 2) bar = bar; else bar = 10; return bar; }'
assert 2 'int main() { int bar = 2; if (bar == 1) {bar = bar;} else if (bar == 2) {bar = bar;} else {bar = 10;} return bar; }'
assert 2 'int main() { int bar = 2; if (bar == 1) return bar; return bar; }'
assert 2 'int main() { int bar = 2; if (bar == 1) return bar; else if (bar == 3) return bar; return bar; }'

assert 3 'int main() { return 3; }'
assert 30 'int foo(int a) { return a + 5; } int main() { return foo(10) + foo(10); }'
assert 15 'int bar(int a,int b) { return a * 10 + b; } int main() { return bar(1,5); }'
assert 11 'int bar(int a,int b) { return a * 10 + b; } int main() { int a = bar(1,1); return a; }'
assert 15 'int bar(int a,int b) { int aaa = a * 10 + b;return aaa; } int main() { return bar(1,5); }'
assert 8 'int fib(int a) { if (a <= 1) return 1; else return fib(a-1) + fib(a-2); } int main() { return fib(5); }'



assert 7 'int bar2(int a,int b) { return a + b; } int bar3(int a,int b,int c) { return a + b + c; } int main() { int a = 3; int b = 4; return bar2(a,b); }'
assert 12 'int bar2(int a,int b) { return a + b; } int bar3(int a,int b,int c) { return a + b + c; } int main() { int a = 3; int b = 4; int c = 5; return a + b + c; }'
assert 24 'int bar2(int a,int b) { return a + b; } int bar3(int a,int b,int c) { return a + b + c; } int main() { int a = 3; int b = 4; int c = 5; return a + b + c + bar3(a,b,c); }'
assert 19 'int bar2(int a,int b) { return a + b; } int bar3(int a,int b,int c) { return a + b + c; } int main() { int a = 3; int b = 4; int c = 5; return bar2(a,b) + bar3(a,b,c); }'
assert 20 'int bar2(int a,int b) { return a + b; } int bar3(int a,int b,int c) { return a + b + c; } int main() { int a = 3; int b = 4; int c = 5; return bar2(a,c) + bar3(a,b,c); }'
assert 21 'int bar2(int a,int b) { return a + b; } int bar3(int a,int b,int c) { return a + b + c; } int main() { int a = 3; int b = 4; int c = 5; return bar2(c,b) + bar3(a,b,c); }'

#assert 42 '42;'
#assert 41 ' 12 + 34 - 5; '
#assert 15 '5*(9-6);'
assert 4 'int main() { return (3 + --5) / 2; }'
assert 4 'int main() { return (14 + -5) / 2; }'
#assert 38 '-(10--9)*-2;'
#assert 10 '- -10;'
#assert 10 '- - +10;'

#assert 0 '0==1;'
assert 1 'int main() { return 42==42; }'
assert 1 'int main() { return 0!=1; }'
#assert 0 '42!=42;'

#assert 1 '0<1;'
#assert 0 '1<1;'
#assert 0 '2<1;'
#assert 1 '0<=1;'
#assert 1 '1<=1;'
#assert 0 '2<=1;'

#assert 1 '1>0;'
#assert 0 '1>1;'
#assert 0 '1>2;'
#assert 1 '1>=0;'
#assert 1 '1>=1;'
#assert 0 '1>=2;'

assert 14 'int main() { int a = 3; int b = 5 * 6 - 8; return a + b / 2; }'

assert 6 'int main() { int foo = 1; int bar = 2 + 3; return foo + bar; }'

assert 14 'int main() { int foo = 3; int bar = 5 * 6 - 8; return foo + bar / 2; }'
assert 5 'int main() { return 5; return 8; }'

assert 3 'int main() { int a = 3; if (2 <= a) return a; else return 0; }'
assert 5 'int main() { int a = 4; int b = 0; while (b <= a) b = b + 1; return b; }'
assert 10 'int main() { int bar = 0; for (int foo = 1; foo < 5; foo = foo + 1) bar = bar + foo; return bar; }'
assert 1 'int main() { int bar = 1; if (bar == 1) return 1; return bar; }'
assert 3 'int main() { int bar = 3; if (bar == 0) return 0; else return bar; }'
assert 10 'int main() { int bar = 1; if (bar == 0) bar = 1; else if (bar > 0) bar = bar * 10; return bar; }'
assert 102 'int main() { int bar = 2; if (bar == 0) return 100; else if (bar == 1) return 101; else if (bar == 2) return 102; else return bar; }'

assert 0 'int main() { int bar = 3; if (bar == 3) { bar = bar - 1; bar = bar - 1; bar = bar - 1; } else bar = 10; return bar; }'
assert 10 'int main() { int bar = 4; if (bar == 3) { bar = bar - 1; bar = bar - 1; bar = bar - 1; } else bar = 10; return bar; }'

echo OK
