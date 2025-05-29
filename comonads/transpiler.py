#!/usr/bin/env python3

def foo():
    print("foo")

# The worst python interpeter
with open("code.py") as file:
    for line in file:
        print(":", line.strip())
        exec(line)