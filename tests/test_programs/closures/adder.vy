fn make_adder(x) {
	return fn(y) {
		return x + y
	}
}

const add10 = make_adder(10)
return add10(-10)
