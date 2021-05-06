const t = {
  f(x) {
    return {
      f(y) {
        return x + 2
      }
    }
  }
}

return t:f(3):f(2)