fn map(str, f) {
  for i = 0, #str {
    f(str:code_at(i))
  } 
}

let sum = 0
map('abcdef', fn (char_code) {
  sum += char_code
})

return sum