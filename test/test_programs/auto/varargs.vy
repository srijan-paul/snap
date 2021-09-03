fn add(xs...) {
	return xs:reduce(/x, y -> x + y)
}

const mult = /xs... -> xs:reduce(/x, y -> x * y)

assert(add(1, 2) == 3 && add(1, 2, 3) == 6, "varargs broken")
assert(mult(1, 2) == 2 && mult(1, 2, 3) == 6, "varargs in arrow functions broken")

const scale_and_reduce = /a, xs... -> a * xs:reduce(/x, y -> x + y)

assert(scale_and_reduce(2, 1, 2, 3) == 12, "varargs preceding an argument broken")