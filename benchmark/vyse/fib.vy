fn fib(n) {
  if n < 2 { return n }
  return fib(n - 1) + fib(n - 2)
}

for i = 0, 5 {
  print(fib(28))
}