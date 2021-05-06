fn make_adder(x) {
	return fn() {
		let z = 1
		let w = 2
		return 3 + z + w
	}
}

const add10 = make_adder()
return add10()