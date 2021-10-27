fn foo(a, b) {
  return a + b
}

assert(foo(1, 2, 3, 4, 5, 6, 7, 8) == 3)

const id = /(x) -> x
assert(id(1) == 1)
assert(id(1, 2, 3, 4) == 1)

const g = /(x, y, z) -> {
  if z { return x + y + z }
  if y { return x + y }
  return x
} 

assert(g(1, 2, 13) == 16)
assert(g(1, 2) == 3)
assert(g(56) == 56)
