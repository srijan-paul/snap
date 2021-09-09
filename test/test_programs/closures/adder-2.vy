fn adder(x) {
	fn add(y) {
		return x + y
	}	
	return add
}
return adder(10)(20)
