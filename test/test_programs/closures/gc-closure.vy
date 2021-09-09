fn make_adder(x) {
  return fn (y) {
    return x + y
  }
}

let adder = make_adder(10)
adder = nil
adder = make_adder(30)

return adder(10)
