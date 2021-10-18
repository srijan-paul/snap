fn baz(x) {
  return { a: x, b: x * 2 }
}

fn bar(x) {
  return baz(x * 2)
}

fn foo(x) {
  return bar(x * 2)
}

return foo(1).a + foo(2)['b']
