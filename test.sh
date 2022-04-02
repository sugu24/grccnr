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

assert 42 '42;'
assert 41 ' 12 + 34 - 5; '
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'
assert 38 '-(10--9)*-2;'
assert 10 '- -10;'
assert 10 '- - +10;'

assert 0 '0==1;'
assert 1 '42==42;'
assert 1 '0!=1;'
assert 0 '42!=42;'

assert 1 '0<1;'
assert 0 '1<1;'
assert 0 '2<1;'
assert 1 '0<=1;'
assert 1 '1<=1;'
assert 0 '2<=1;'

assert 1 '1>0;'
assert 0 '1>1;'
assert 0 '1>2;'
assert 1 '1>=0;'
assert 1 '1>=1;'
assert 0 '1>=2;'

assert 14 'a = 3; b = 5 * 6 - 8; a + b / 2;'

assert 6 'foo = 1; bar = 2 + 3; foo + bar;'

assert 14 'foo = 3; bar = 5 * 6 - 8; return foo + bar / 2;'
assert 5 'return 5; return 8;'

echo OK
