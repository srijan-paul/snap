fn make_tree(item, depth) {
  if depth > 0 {
    const item2 = item + item
    depth -= 1
    const left = make_tree(item2 - 1, depth)
    const right = make_tree(item2, depth)
    return [ item, left, right ]
  } else {
    return [ item, nil, nil ]
  }
}

fn check_tree(tree) {
  if tree[1] {
    return tree[0] + check_tree(tree[1]) - check_tree(tree[2])
  } else {
    return tree[0]
  }
}

const min_depth = 4
const max_depth = 12

{
  const stretch_depth = max_depth + 1
  const stretch_tree = make_tree(0, stretch_depth)
  print('stretch tree of depth',
    stretch_depth, 'check:', check_tree(stretch_tree))
}

const long_lived_tree = make_tree(0, max_depth)

let iterations = 2 ** max_depth
for depth = min_depth, max_depth + 1, 2 {
  let check = 0
  for i = 0, iterations {
    check = check + check_tree(make_tree(1, depth)) + 
      check_tree(make_tree(-1, depth))
  }
  print(iterations*2, 'trees of depth', depth, 'check:', check)
  iterations /= 4
}

print('long lived tree of depth', max_depth,
  'check: ', check_tree(long_lived_tree))