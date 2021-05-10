-- left fold
fn fold(list, init, reducer) {
  for i = 0, #list {
    init = reducer(init , list[i])
  }
  return init
}


const add  = fn (x, y) { return x + y }
assert(fold([1, 2, 3, 4, 5], 0, add) == 15)