fn x() {
	let a = 1
	let b = 2
	fn y() {
		let c = 3
		fn z() {
			return a + b + c;
		}
		return z
	}
	return y
}
return x()()()
