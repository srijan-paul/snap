let x = 0
for i = 1, 10  {
  if (i % 2 == 0) continue;
  x += i
}
-- expect 25
return x