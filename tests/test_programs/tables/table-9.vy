const A = {
  B: {
    C: {
      D: 10
    }
  }
}

return A.B.C.D + A['B']['C'].D + A.B['C'].D
