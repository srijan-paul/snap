let j = 0
let i = 0
let k = 5

while true {
  let a = 1
  let b = 2
  i += a
  if i >= 10 break
  let c = 4
  let d = 4
  i += c - d
}

while true {
  let a = 1
  let b = 2
  j += a
  if j >= 10 break
  let c = 4
  let d = 4
  j += c - d
}

-- expect: 25 
return i + j + k