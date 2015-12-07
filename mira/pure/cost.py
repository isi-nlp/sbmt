import math
_LOG_10 = math.log(10)

try:
    IMPOSSIBLE=float('inf')
except ValueError:
    IMPOSSIBLE=1000000.

def fromprob(p):
    try:
        return -math.log10(p)
    except (ValueError, OverflowError):
        return IMPOSSIBLE

def prob(x):
    return math.pow(10.,-x)

def add(x, y):
    if y > x:
        return x-math.log1p(math.pow(10, x-y)) / _LOG_10
    else:
        return y-math.log1p(math.pow(10, y-x)) / _LOG_10

def subtract(x, y):
    if y-x > 1:
        return x-math.log1p(-math.pow(10, x-y)) / _LOG_10
    else:
        return x-math.log10(-math.expm1((x-y)*_LOG_10))

if __name__ == "__main__":
    p1 = 0.9
    p2 = 0.5
    print p1+p2, prob(add(fromprob(p1), fromprob(p2)))
    print p1-p2, prob(subtract(fromprob(p1), fromprob(p2)))
