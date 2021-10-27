const twice = /(x) -> x * 2
assert(twice(10) == 20)

const add = /(x, y) -> x + y
assert(add(1, 2) == 3)

const mult = /(a, b) -> {
  let res = a * b
  return res
}

assert(mult(2, 3) == 6)
