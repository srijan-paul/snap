const t = {
  a: 1,
  b: 2,
  c: 3
}
const t2 = { k: 1 }
let x = #t + #t2
t.a = nil
t.b = nil
x += #t

-- expect 5
return x
