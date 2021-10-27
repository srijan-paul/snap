let x = 0
for i = 1, 10  {
  if (i == 8) break;
  x += i
}
-- expect 28 
return x