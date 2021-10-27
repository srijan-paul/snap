const t = {
  x: 1,
  y: 2,
  getx() {
    return self.x
  },

  gety() {
    return self.y
  }
}

const mt = {
  z: 3,
  getz() {
    return self.z
  }
}

setproto(t, mt)
return t:getx() + t:gety() + t:getz()