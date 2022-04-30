#!/bin/bash
assert(){
	expected="$1"
	input="$2"

	./grccnr "$input" > temp.s
	cc -o temp.o -c temp.s
	cc -o temp temp.o link.o
	./temp
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$input => $expected expected, but got $actual"
		exit 1
	fi
}

assert 29 testcase/test13.gc
assert 100 testcase/test12.gc
assert 122 testcase/test11.gc
assert 10 testcase/test10.gc
assert 0 testcase/test9.gc
assert 2 testcase/test8.gc
assert 0 testcase/test7.gc
assert 0 testcase/test6.gc
assert 0 testcase/test5.gc
assert 0 testcase/test1.gc
assert 11 testcase/test3.gc
assert 34 testcase/test2.gc
assert 0 testcase/test4.gc
assert 100 testcase/end.gc

assert 0 'int f(int *a) { int i; for (i = 0; i < 4; i = i + 1) printf("%d \n", a[i]); } int main() { int a[4]; int i; for (i = 0; i < 4; i = i + 1) a[i] = i; f(a); }'
assert 0 'int f(int a[4]) { int i; for (i = 0; i < 4; i = i + 1) printf("%d \n", a[i]); } int main() { int a[4]; int i; for (i = 0; i < 4; i = i + 1) a[i] = i; f(a); }'
assert 1 'int main() { int a[4][3]; a[3][2] = 1; return a[3][2]; }'

assert 97 'int main() { char a; char b[3]; b[0] = 97; a = b[0]; return a; }'
assert 97 'int main() { char a; char b[3]; b[0] = 97; b[1] = 98; a = b[0]; printf("%s\n", b); return a; }'

assert 0 'int main() { int a; int b; a = 1; b = 2; printf("a=%d, b=%d, a+b=%d\n", a, b, a+b); return 0; }'
assert 1 'int main() { int a; a = 1; printf("a is %d.\n", a); return a; }'
assert 3 'int a[3]; int main() { a[0] = 1; a[1] = 2; a[2] = a[0] + a[1]; return a[2]; }'
assert 3 'int a; int b; int main() { a = 3; b = a; return b; }'
assert 0 'char *str1; char *str2; int main() { str1 = "aa"; str2 = str1; printf("str2 = %s\n", str2); return 0; }'
assert 0 'char *str; int main() { str = "aaaaa"; printf("str = %s\n", str); return 0; }'

assert 3 'int main() { int a; a = 1 + 1; a = a + 1; return a; }'

assert 4 'int a; int main() { return sizeof(a); }'
assert 1 'char a; int main() { return sizeof(a); }'
assert 8 'int *a; int main() { return sizeof(a); }'
assert 1 'int main() { char x[3]; x[0] = 1; return x[0]; }'
assert 1 'int main() { char x[3]; x[0] = 1; int y; y = 1; return x[0]; }'
assert 3 'int main() { char x[3]; x[0] = -1; int y; y = 4; return x[0] + y; }'
assert 3 'int main() { char x[3]; x[1] = -1; x[0] = x[1]; int y; y = 4; return x[0] + y; }'
assert 3 'int main() { char x[3]; *(x+1) = -1; x[0] = x[1]; int y; y = 4; return x[0] + y; }'

assert 0  'int x; int y[20]; int main() { return x + y[5]; + y[0]; } '
assert 1 'int main() { int a[3]; int two; two = 2; a[two] = 1; int y; y = a[two]; return y; }'
assert 3 'int main() { int a[2]; *a = 1; *(a+1) = 2; int *p; p = a; return *p+*(p+1); }'
assert 2 'int main() { int a[2]; *(a+1) = 2; return *(a+1); }'
assert 2 'int main() { int *a; int y; y = 2; a = &y; return *a; }'
assert 3 'int main() { int x; int *y; y = &x; *y = 3; return x;}'
assert 1 'int main() { int a[10]; a[0] = 1; return a[0]; }'
assert 4 'int main() { int a; a = 1; int b; b = 2; int c; c = 3; int *d; d = &c; int **e; e = &d; *(*e+1) = 4; return b;}'


assert 4 'int main() { int x; int *y; return sizeof(x); }'
assert 8 'int main() { int x; int *y; return sizeof(y); }'
assert 4 'int main() { int x; int *y; return sizeof(x+3); }'
assert 8 'int main() { int x; int *y; return sizeof(y+3); }'
assert 4 'int main() { int x; int *y; return sizeof(*y); }'
assert 4 'int main() { int x; int *y; return sizeof(1); }'
assert 4 'int main() { int x; int *y; return sizeof(sizeof(1)); }'

assert 11 'int foo() { return 11; } int bar(int a,int b) { return a * 10 + b; } int main() { int c; c = bar(1,1); return c; }'
assert 11 'int bar(int a,int b) { return a * 10 + b; } int main() { int c; c = bar(1,1); return c; }'
assert 4 'int main() { int *a; int b; b = 4; a = &b; *a = *a - 1; *a = *a - 1; return 2*b;}'
assert 2 'int main() { int a; a = 1; int b; b = 1; return b + 1; }'


assert 4 'int bar(int *x) { *x = 4; } int main() { int *a; int b; b = 1; a = &b; bar(a); return *a; }'
assert 10 'int bar(int **x) { int y; y = 10; *x = &y; } int main() { int *a; int b; b = 1; a = &b; bar(&a); return *a; }'

assert 3 'int main() { int x; x = 3; int *y; y = &x; return *y; }'
assert 3 'int main() { int x; x = 3; int y; y = 5; int *z; z = &y + 1; return *z; }'

assert 20 'int main() { int bar; bar = 2; if (bar == 2) bar = bar * 10; return bar; }'
assert 1 'int main() { int bar; bar = 0; if (bar == 0) bar = 1; else if (bar > 0) bar = bar * 10; return bar; }'
assert 10 'int main() { int bar; bar = 0; if (bar == 1) bar = bar; else if (bar == 2) bar = bar; else bar = 10; return bar; }'
assert 1 'int main() { int bar; bar = 1; if (bar == 1) bar = bar; else if (bar == 2) bar = bar; else bar = 10; return bar; }'
assert 2 'int main() { int bar; bar = 2; if (bar == 1) {bar = bar;} else if (bar == 2) {bar = bar;} else {bar = 10;} return bar; }'
assert 2 'int main() { int bar; bar = 2; if (bar == 1) return bar; return bar; }'
assert 2 'int main() { int bar; bar = 2; if (bar == 1) return bar; else if (bar == 3) return bar; return bar; }'

assert 3 'int main() { return 3; }'
assert 30 'int foo(int a) { return a + 5; } int main() { return foo(10) + foo(10); }'
assert 15 'int bar(int a,int b) { return a * 10 + b; } int main() { return bar(1,5); }'
assert 11 'int bar(int a,int b) { return a * 10 + b; } int main() { int a; a = bar(1,1); return a; }'
assert 15 'int bar(int a,int b) { int aaa; aaa = a * 10 + b; return aaa; } int main() { return bar(1,5); }'
assert 8 'int fib(int a) { if (a <= 1) return 1; else return fib(a-1) + fib(a-2); } int main() { return fib(5); }'



assert 7 'int bar2(int a,int b) { return a + b; } int bar3(int a,int b,int c) { return a + b + c; } int main() { int a; a = 3; int b; b = 4; return bar2(a,b); }'
assert 12 'int bar2(int a,int b) { return a + b; } int bar3(int a,int b,int c) { return a + b + c; } int main() { int a; a = 3; int b; b = 4; int c; c = 5; return a + b + c; }'
assert 24 'int bar2(int a,int b) { return a + b; } int bar3(int a,int b,int c) { return a + b + c; } int main() { int a; a = 3; int b; b = 4; int c; c = 5; return a + b + c + bar3(a,b,c); }'
assert 19 'int bar2(int a,int b) { return a + b; } int bar3(int a,int b,int c) { return a + b + c; } int main() { int a; a = 3; int b; b = 4; int c; c = 5; return bar2(a,b) + bar3(a,b,c); }'
assert 20 'int bar2(int a,int b) { return a + b; } int bar3(int a,int b,int c) { return a + b + c; } int main() { int a; a = 3; int b; b = 4; int c; c = 5; return bar2(a,c) + bar3(a,b,c); }'
assert 21 'int bar2(int a,int b) { return a + b; } int bar3(int a,int b,int c) { return a + b + c; } int main() { int a; a = 3; int b; b = 4; int c; c = 5; return bar2(c,b) + bar3(a,b,c); }'

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

assert 14 'int main() { int a; a = 3; int b; b = 5 * 6 - 8; return a + b / 2; }'

assert 6 'int main() { int foo; foo = 1; int bar; bar = 2 + 3; return foo + bar; }'

assert 14 'int main() { int foo; foo = 3; int bar; bar = 5 * 6 - 8; return foo + bar / 2; }'
assert 5 'int main() { return 5; return 8; }'

assert 3 'int main() { int a; a = 3; if (2 <= a) return a; else return 0; }'
assert 5 'int main() { int a; a = 4; int b; b = 0; while (b <= a) b = b + 1; return b; }'
assert 10 'int main() { int bar; int foo; bar = 0; for (foo = 1; foo < 5; foo = foo + 1) bar = bar + foo; return bar; }'
assert 1 'int main() { int bar; bar = 1; if (bar == 1) return 1; return bar; }'
assert 3 'int main() { int bar; bar = 3; if (bar == 0) return 0; else return bar; }'
assert 10 'int main() { int bar; bar = 1; if (bar == 0) bar = 1; else if (bar > 0) bar = bar * 10; return bar; }'
assert 102 'int main() { int bar; bar = 2; if (bar == 0) return 100; else if (bar == 1) return 101; else if (bar == 2) return 102; else return bar; }'

assert 0 'int main() { int bar; bar = 3; if (bar == 3) { bar = bar - 1; bar = bar - 1; bar = bar - 1; } else bar = 10; return bar; }'
assert 10 'int main() { int bar; bar = 4; if (bar == 3) { bar = bar - 1; bar = bar - 1; bar = bar - 1; } else bar = 10; return bar; }'

echo OK
