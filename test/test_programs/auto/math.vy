const math = import("math")

assert(math.pi / 2 == math.torad(90))
assert(math.log(5) / math.log(4) == math.log(5, 4))
assert(math.pow(5, 3, 2) == math.pow(5, 3) % 2)
assert(math.pow(1414, 21, 1231231) == 1017565)

assert(math.comb(51, 4) == 249900);

{
	assert(math.floor(5.34213) == 5)
	assert(math.ceil(5.21341) == 6)
	assert(math.floor(-5.34213) == -6)
	assert(math.ceil(-5.34213) == -5)
	assert(math.ceil(math.min_int) == math.min_int)
	assert(math.floor(math.min_int) == math.min_int)
}