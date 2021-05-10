-- something the matter with this :( 

fn make_tree(item, depth) {
  if depth == 0 {
    return [item, nil, nil]
  }

  const item2 = item + item
  depth -= 1
  return [item, make_tree(item2 - 1, depth), make_tree(item2, depth)]
}

fn check_tree(node) {
  return 1
}

const min_depth = 4
const max_depth = 12
const stretch_depth = max_depth + 1

print("stretch tree of depth", stretch_depth, check_tree(make_tree(0, stretch_depth)))

const long_lived_tree = make_tree(0, max_depth)

let iterations = 2 ** max_depth;

for depth = min_depth, stretch_depth, 2 {
  let check = 0
  for i = 1, iterations + 1 {
    check += check_tree(make_tree(i, depth)) + check_tree(make_tree(-i, depth))
  }
  print(iterations * 2, 'trees of depth', depth, 'check:', check)
  iterations /= 4
}

print('longed lived tree of depth', max_depth, 'check:', check_tree(long_lived_tree))
