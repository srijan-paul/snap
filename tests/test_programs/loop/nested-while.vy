let i = 0
let x = 0

while (i < 10) {
  let j = 0
  while (j < i) {
    x += j
    j += 1
  }
  x += i
  i += 1
}

return x