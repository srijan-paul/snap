let a = 1
let b = 2

fn f() {
  let d
  {
    let c = a + b
    d = c * 2
  }
  return d
}

print(f())