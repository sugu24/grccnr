#!/bin/bash
assert() {
    input="$1"

    ./cinter "$input"
}

assert "test"