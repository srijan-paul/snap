const list = []

for i = 0, 1000000 {
  list <<< i
}

let sum = 0
for i = 0, #list {
  sum += i
}

print(sum)