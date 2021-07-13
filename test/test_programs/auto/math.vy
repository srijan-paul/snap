const math = import("math")

assert(math.pi / 2 == math.torad(90))
assert(math.log(5) / math.log(4) == math.log(5, 4))
assert(math.pow(5, 3, 2) == math.pow(5, 3) % 2)
assert(math.pow(1414, 21, 1231231) == 1017565)

assert(math.comb(51, 4) == 249900);