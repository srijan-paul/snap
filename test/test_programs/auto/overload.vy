const Vec2 = {
  make(x, y) {
    return setproto({ x: x, y: y }, self)
  },

  check(x, y) {
    assert(self.x == x && self.y == y, "check failed")
  }
}

Vec2.__add = fn (left, right) {
  return Vec2:make(left.x + right.x, left.y + right.y)
}

const a = Vec2:make(1, 2)
a:check(1, 2)

const b = Vec2:make(3, 5)
b:check(3, 5)

let c = a + b
c:check(4, 7)

Vec2.__add(a, b):check(4, 7)

a:__add(b):check(4, 7)
b:__add(a):check(4, 7)


const T = {
  new(x) {
    return setproto({x: x}, self)
  },

  __add(other) {
    return self:new(self.x + other.x)
  },

  __sub(other) {
    return self:new(self.x - other.x)
  },

  __mult(other) {
    return self:new(self.x * other.x)
  }
}

let t1 = T:new(1)
let t2 = T:new(2)
assert((t1 + t2).x == 3)
assert((t1 - t2).x == -1)
assert((t1 * t2).x == 2)