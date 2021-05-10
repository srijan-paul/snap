const arr = [1, 2, 3]

let k = 0;
arr:foreach(fn (x) {
  arr:foreach(fn (x) {
    k += 1;
    arr:foreach(print)
  })
})

