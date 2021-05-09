fn bottles(n) {
  for i = n, 0, -1 {
    const word = i == 1 && ' bottle ' || ' bottles ';
    print(i:to_string() .. word ..  ' of beer on the wall.')
    print('take one down, pass it around.')
    print()
  }

  print('No more bottles of beer on the wall!')
}

bottles(99)