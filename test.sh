#!/bin/bash
assert(){
	expected="$1"
	input="$2"

	./grccnr "$input" > temp.s
	cc -o temp temp.s
	./temp
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$input => $expected expected, but got $actual"
		exit 1
	fi
}
assert 3 'main() { x = 3; y = &x; return *y; }'
assert 3 'main() { x = 3; y = 5; z = &y + 8; return *z; }'

assert 20 'main() { bar = 2; if (bar == 2) bar = bar * 10; return bar; }'
assert 1 'main() { bar = 0; if (bar == 0) bar = 1; else if (bar > 0) bar = bar * 10; return bar; }'
assert 10 'main() { bar = 0; if (bar == 1) bar = bar; else if (bar == 2) bar = bar; else bar = 10; return bar; }'
assert 1 'main() { bar = 1; if (bar == 1) bar = bar; else if (bar == 2) bar = bar; else bar = 10; return bar; }'
assert 2 'main() { bar = 2; if (bar == 1) {bar = bar;} else if (bar == 2) {bar = bar;} else {bar = 10;} return bar; }'
assert 2 'main() { bar = 2; if (bar == 1) return bar; return bar; }'
assert 2 'main() { bar = 2; if (bar == 1) return bar; else if (bar == 3) return bar; return bar; }'

assert 3 'main() { a = 1; b = 2; return a + b; }'
assert 3 'main() { return 3; }'
assert 30 'foo(a) { return a + 5; } main() { return foo(10) + foo(10); }'
assert 15 'bar(a,b) { return a * 10 + b; } main() { return bar(1,5); }'
assert 11 'bar(a,b) { return a * 10 + b; } main() { c = bar(1,1); return c; }'
assert 11 'bar(a,b) { return a * 10 + b; } main() { a = bar(1,1); return a; }'
assert 15 'bar(a,b) { aaa = a * 10 + b;return aaa; } main() { return bar(1,5); }'
assert 8 'fib(a) { if (a <= 1) return 1; else return fib(a-1) + fib(a-2); } main() { return fib(5); }'



assert 7 'bar2(a,b) { return a + b; } bar3(a,b,c) { return a + b + c; } main() { a = 3; b = 4; return bar2(a,b); }'
assert 12 'bar2(a,b) { return a + b; } bar3(a,b,c) { return a + b + c; } main() { a = 3; b = 4; c = 5; return a + b + c; }'
assert 24 'bar2(a,b) { return a + b; } bar3(a,b,c) { return a + b + c; } main() { a = 3; b = 4; c = 5; return a + b + c + bar3(a,b,c); }'
assert 19 'bar2(a,b) { return a + b; } bar3(a,b,c) { return a + b + c; } main() { a = 3; b = 4; c = 5; return bar2(a,b) + bar3(a,b,c); }'
assert 20 'bar2(a,b) { return a + b; } bar3(a,b,c) { return a + b + c; } main() { a = 3; b = 4; c = 5; return bar2(a,c) + bar3(a,b,c); }'
assert 21 'bar2(a,b) { return a + b; } bar3(a,b,c) { return a + b + c; } main() { a = 3; b = 4; c = 5; return bar2(c,b) + bar3(a,b,c); }'

#assert 42 '42;'
#assert 41 ' 12 + 34 - 5; '
#assert 15 '5*(9-6);'
assert 4 'main() { return (3 + - -5) / 2; }'
#assert 38 '-(10--9)*-2;'
#assert 10 '- -10;'
#assert 10 '- - +10;'

#assert 0 '0==1;'
assert 1 'main() { return 42==42; }'
assert 1 'main() { return 0!=1; }'
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

assert 14 'main() { a = 3; b = 5 * 6 - 8; return a + b / 2; }'

assert 6 'main() { foo = 1; bar = 2 + 3; return foo + bar; }'

assert 14 'main() { foo = 3; bar = 5 * 6 - 8; return foo + bar / 2; }'
assert 5 'main() { return 5; return 8; }'

assert 3 'main() { a = 3; if (2 <= a) return a; else return 0; }'
assert 5 'main() { a = 4; b = 0; while (b <= a) b = b + 1; return b; }'
assert 10 'main() { bar = 0; for (foo = 1; foo < 5; foo = foo + 1) bar = bar + foo; return bar; }'
assert 1 'main() { bar = 1; if (bar == 1) return 1; return bar; }'
assert 3 'main() { bar = 3; if (bar == 0) return 0; else return bar; }'
assert 10 'main() { bar = 1; if (bar == 0) bar = 1; else if (bar > 0) bar = bar * 10; return bar; }'
assert 102 'main() { bar = 2; if (bar == 0) return 100; else if (bar == 1) return 101; else if (bar == 2) return 102; else return bar; }'

assert 0 'main() { bar = 3; if (bar == 3) { bar = bar - 1; bar = bar - 1; bar = bar - 1; } else bar = 10; return bar; }'
assert 10 'main() { bar = 4; if (bar == 3) { bar = bar - 1; bar = bar - 1; bar = bar - 1; } else bar = 10; return bar; }'

echo OK
