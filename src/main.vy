fn bottles() {
  for total = 99, 1, -1 {
    const word = (total == 1) && " bottle" || " bottles";
    print(total:to_string() .. word .. " of beer on the wall !")
    print("Take one down, pass it around!")
    print()
  }

  print("No more bottles of beer on the wall!")
}

bottles()
