const s1 = 'abc'
const s2 = 'def'

const T = {
  abc: 1,
  ['def']: 2,
  abcdef: 3
}

return T[s1] + T[s2] + T[s1..s2]