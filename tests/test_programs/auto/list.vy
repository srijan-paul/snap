const arr = [1, 2, 3, 4, 5]

let sum = 0;
for i = 0, #arr {
  sum += arr[i];
}

arr <<< 1 <<< 2 * 3 <<< 4 | 5 <<< 7 ^ 8;
let new_sum = 0
for i = 0, #arr {
  new_sum += arr[i]
}

const diff =  1 + 2 * 3 + (4 | 5) + (7 ^ 8)
assert(new_sum - sum == diff) 


const arr2 = [] <<< [1, 2] <<< [3, 4]
assert(#arr2 == 2)
assert(#arr2[0] == 2)
let k = 1
for i = 0, #arr2 {
  for j = 0, #arr2[i] {
    assert(arr2[i][j] == k)
    k += 1
  }
}

let xs = [1, 2, 3, 4]
xs[0] += 1
assert(xs[0] == 2, "'+=' on list indices")
