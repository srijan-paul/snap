fn bottles() {
  let total = 99
  while true {
    print(total)
    print((total == 1 && "bottle" || "bottles") .. " of beer on the wall !")
    total -= 1
    if (total > 0)
      print("Take one down, pass it around!")
    else 
      break
  }

  print("No more bottles of beer on the wall!")
}

bottles()