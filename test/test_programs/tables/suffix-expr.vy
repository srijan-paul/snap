const t = {
  t: {
    f() {
      return {
        ['123']: {
          x: {
            ['y']: {
              ['z']: 123
            }
          }
        }
      }
    }
  }
}

return t.t:f()['123'].x['y']['z']