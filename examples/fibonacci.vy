fn fib(n) {
  let a = 1
  let b = 1
  let c

  while n > 0 {
    c = a + b
    a = b
    b = c
  }

  return c
}

print(fib(10))