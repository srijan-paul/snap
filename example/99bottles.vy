fn bottles() {
  let total = 99
  while total > 0 {
    const word = (total == 1) && " bottle" || " bottles";
    print(total:to_string() .. word .. " of beer on the wall !")
    print("Take one down, pass it around!")
    print()
    total -= 1
  }

  print("No more bottles of beer on the wall!")
}

bottles()