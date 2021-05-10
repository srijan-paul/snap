const a = [1, 2, 3, 4, 5, 6, 7, 8]
const b = [0, 0]
let s = ''

b:foreach(fn () {
  a:foreach(fn (x) {
    s = s .. x:to_string()
  })
})

assert(s == '1234567812345678', 'list-foreach and string comparisons')