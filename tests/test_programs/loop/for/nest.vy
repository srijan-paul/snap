let x = 0
for i = 1, 10 
  for j = 1, i
    x += i * j

-- expect 870
return x