let k = 1

for i = 10, 1, -1 {
  let z = 2 * i 
  k += i + z
}

-- expect: 166
return k