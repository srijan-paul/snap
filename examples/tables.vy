const child = {
  name: "Bobo",
  age: 10
}

const parent = {
  name: "Papa",
  age: 35,
  race: "Human"
}

setproto(child, parent)
print(child.race)