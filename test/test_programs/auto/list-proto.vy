const a = [1, 2, 3, 4, 5, 6, 7, 8]
const b = [0, 0]
let s = ''

b:foreach(fn () {
  a:foreach(fn (x) {
    s = s .. x:to_string()
  })
})

assert(s == '1234567812345678', 'list-foreach and string comparisons')

const arr = List.make(100)
assert(#arr == 100, "List.make() doesn't work as expected.")

const lst = List.make(20)
assert(#lst == 20, "List.make() doesn't work.")
lst:fill(-1)
lst:foreach(fn(x) {
  assert(x == -1, "List.fill() doesn't work")
})