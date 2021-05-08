const t = {}
assert(!t.x)

const pt = { x: 1 }
setproto(t, pt)
assert(t.x == 1, "setproto builtin fails.")

assert(getproto(t) == pt, "getproto builtin failse")
assert(getproto(t).x == pt.x)