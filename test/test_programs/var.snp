let sum = 0
const x = 1, y = 2, z = 3

sum += x
{
  const x = 4
  sum += x
  sum += y
}
sum += x

sum += y
sum += z

-- expect: 13
return sum