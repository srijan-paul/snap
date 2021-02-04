import timeit

def fib(n):
  if n <= 1: return 1
  return fib(n - 1) + fib(n - 2)

def fibtest():
  t_start = timeit.default_timer()
  for i in range(1, 100):
    for i in range(1, 20):
      fib(i)
  return 1000 * (timeit.default_timer() - t_start)


print("Fibonacci python (ms): ", fibtest())