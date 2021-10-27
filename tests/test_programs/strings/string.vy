const a = 'snap'

let c = ''
{
  const b = a
  const a = ' = good'
  c = c..b..a
}

return c
