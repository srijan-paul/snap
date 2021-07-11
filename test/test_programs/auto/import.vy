assert(__loaders__ && __modulecache__)

__loaders__ <<< fn (name) {
    return { modname: name }
}


const module = import("stuff")

assert(__modulecache__.stuff)
assert(module.modname == "stuff")

const math = import("math")

assert(math.sqrt(100) == 10)
