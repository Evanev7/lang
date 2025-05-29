# you can write to stdout for debugging purposes, e.g.
# print("this is a debug message")

def solution(A):
    # Each integer can occur at most twice, except the maximum which can occur exactly once.
    # N >= 1 so we have at least one integer.
    m = max(A)
    sol = {}
    out = 0
    # O(n)
    for i in A:
        if i not in sol.keys():
            sol[i] = 1
            out += 1
        else:
            sol[i] += 1
            if sol[i] == 2:
                out += 1

    # Example 3
    if sol[m] >= 2:
        out -= 1
    
    return out

if __name__ == "__main__":
    print(list(i for i in range(100,000)))
    solution(list(i for i in range(100,000)))