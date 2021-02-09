fn make_table() {
  return {
    number: 1,
    inc: fn(t) {
      t.number = t.number + 1
      return t.number
    }
  }
}

let table = make_table()
return table.number + table.inc(table)
