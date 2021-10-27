fn sum(x) {
  let sum = 0
  for i = 0, x + 1 {
    sum += i
  }
  return sum
}

-- expect 110
return sum(10) + sum(10);