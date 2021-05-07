let x = 1
let y = 2

for i = 1, 11 {
  let z = x + 1
  let w = i + y * i
  x += i + z
  y -= x + w
}

-- expect: 12379
return x - y
