const t1 = { a: 1, b: 2 }
const t2 = { b: 3, c: 4 }
const t3 = { c: 5, d: 6 }
const t4 = { c: 7, e: 8 }

setproto(t1, t2)
setproto(t2, t3)
setproto(t3, t4)

return t1.a + t1.b + t1.c + t1.d + t1['e']
