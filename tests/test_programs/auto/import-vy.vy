const Vec2 = import("./overload.vy");

Vec2.log = fn (v) {
  print(v.x, v.y)
}

const o = Vec2:make(0, 1);
const p = (o + Vec2:make(1, 1))
assert(p.x == 1 && p.y == 2)
