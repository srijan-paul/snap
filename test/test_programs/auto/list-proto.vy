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

-- List.make
-- List.fill
const lst = List.make(20)
assert(#lst == 20, "List.make() doesn't work.")
lst:fill(-1)
lst:foreach(fn(x) {
  assert(x == -1, "List.fill() doesn't work")
})

-- List.slice
let xs = [1, 2, 3, 4, 5, 6, 7]
let xsplice = xs:slice(2, 5)
assert(#xsplice == 4, "slice size incorrect.")
for i = 0, #xsplice {
  assert(xsplice[i] == xs[2 + i], "spice doesn't work")
}

-- List.map
xs = [1, 2, 3, 4, 5]
let xs2 = xs:map(/(x) -> 2 * x)
assert(#xs2 == #xs, "map unequal length")
for i = 0, #xs {
  assert(xs2[i] == 2 * xs[i], "map")
}

xs2 = xs:map(/(x, i) -> i * x)
xs:foreach(/(x, i) -> assert(x * i == xs2[i],"map"))

-- List.reduce
xs = [1, 2, 3, 4, 5]
const add = /(x, y) -> x + y
assert(xs:reduce(add) == 15)
assert(List.reduce(xs, add) == 15)
assert(xs:reduce(/(x, y, index) -> x + y, 15) == 30)

-- List.filter
xs = [1, 2, 3, 4, 5]
let fxs = xs:filter(/(x) -> x % 2 == 0)
assert(#fxs == 2)
assert(fxs[0] == 2 && fxs[1] == 4)

-- mapreducefilter

const even_sqr_sum = xs
  :filter(/(x) -> x % 2 == 0)
  :map(/(x) -> x ** 2)
  :reduce(/(x, y) -> x + y)

assert(even_sqr_sum == 20, "filter-map-reduce error")

-- pop

xs = [1, 2, 3, 4]
assert(xs:pop() == 4)
