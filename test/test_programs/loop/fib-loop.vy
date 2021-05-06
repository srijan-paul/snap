fn fib(n) {
  let a = 0
  let b = 1
  while true {
    let temp = b
    b = a + b
    a = temp
    if (n == 1) break
    n -= 1
  }
  return b
}

return fib(10)
