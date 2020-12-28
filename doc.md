# The Snap language specification.

Snap is a relatively fast, dynamically typed scripting language intended for
embedding in C++ applications, like game engines.

## Overview

The overall features of snap can be gauged from this simple example:

```javascript

let mynum = 123; // mutable variables declared with 'let'
mynum = 456; // assignment with `=`.


const fib_base = 1; // variables declared with `const` cannot be mutated
```

## Control Flow.

Snap supports the following control flow statements:
if-else if-else, for, while, do..while.

If statements are very straightforward:

```lua
if condition1 {
  print("1")
} elif condition2 {
  print("2")
} else {
  print("3")
}
```

For loops in snap come with an extra bit of power. The
general for loops look like this:

```lua
for i = 1, 10 {
  print("i = " .. i)
}
```

You can compare this loop with the following C code that does the very same
thing:

```c
for (int i = 0; i < 10; i++) {
  printf("i = %d\n", i);
}
```
If you want the loop to go up by some other amount every step, then you can
specify a third parameter:

```lua
for i = 1, 10, 2 {
  print("i = " .. i)
}
```

Similarly, to count down to 0 from some number, you could use `10, 0, -1`
as the loop parameters instead.

For iterating over an array, this could be one way:

```lua
for i = 1, #my_array {
  print(my_array[i])
}
```

Wherein the value of `i` starts at 1 and goes up by 1 until it's as big
as the size of the array.

However, Snap provides a more convienient way to do it when you're
concerned with the value at the `i`th index and not `i` itself:

```lua
for item in my_array {
  print(item);
}
```

## Functions
Functions are declared using the `fn` keyword and like most languages,
called using the `()` operator.

```js
fn fib(x) {
  if x <= 1 return fib_base
  return fib(x - 1) + fib(x - 2);
}
```

Alternatively, once can declare them like variables using `let` or `const`
keywords.

```js
let greet = fn(name) {
  return "Hello !" .. name; // strings can be concatenated with the `..` operator
}
```

All functions are first class values in Snap
This means functions can be passed around and used just like
any other value.

Functions are can capture their surrounding environment.
In this sense, all functions are closures.

```js
fn make_greeter(name) {
  const greeting = ". Good morning!"
  return fn() {
    return "Hello, " .. name .. greeting;
  }
}

const bob_greeter = make_greeter("Bob");
bob_greeter(); // Hello, Bob. Good morning!
```

## On extra prameters.

Functions can be called with less or more number of arguments
than mentioned in the definition.

1. When called with fewer arguments, the missing arguments are assumed to be
  `nil`.
2. When called with more arguments, the extra parameters are simply ignored.

For example:

```rs
fn log(x) {
  print(x)
}

log(10); // 10
log(); // nil
log(5, 10); // 5
```

## Rest parameter.

Functions can optionally contain a `rest` parameter, which can stand for any
number of parameters. Note that the `rest` parameter must be the last in
parameter list.

```rs
fn log(...xs) {
  for x in xs {
    print(x)
  }
}

log(1, 2, 3) // 1, 2, 3
```

## Objects.
Objects are a data structure that basically behave as a hashtable.
You can think of them as key value pairs just like objects in javascript or
tables in Lua.

```js
const Tom = {
  name: "Tom",
  age: 19,
  race: "cat"
}

print(Tom.name) // Tom
```

Objects can be indexed with any value. Strings, numbers, booleans, functions
and even other tables are all possible key types for an object.
The only exception to this rule is the `nil` value.

```js
print(Tom['age']) // 19.

let my_obj = { a: 1, b: 2 }
Tom[my_obj] = 123;

print(Tom[my_obj]) // 123
```

To remove a value from a table, use the `delete` keyword.
Alternatively, set the value of the key you want to delete to `nil`.

```js
delete Tom my_obj // or delete(Tom, my_obj)
print(Tom[my_obj]) // nil

Tom.age = nil // alternative to delete
```

### Method calls.

Objects can also have methods, which can be called with the `:` operator.
For any method bound to an object, there should always be an extra first
parameter. By convention, the first parameter is called `self`. But you may
choose to call it by any name.

```js
const Cat = {
  name: "Tom",
  sound: "Meow"
  meow(self) {
    print(self.sound .. "!")
  }
}

Cat:meow() // Meow!
```

Note that calling a method with the `:` operator simply means the first
parmeter passed to the function is the caller itself. In this case, it is
exactly identical to saying `Cat.meow(Cat)`.

## Parent objects.

An object can have parent objects, when a certain property is not found
in an object itself, the parent is queried for the property.

```js
const t_parent = { a: 1, b: 2 };
const t_child = { a: 3 };

print(t_child.a, t_child.b) // 3, nil
setparent(t_child, t_parent) // set t_parent as t_child's child
print(t_child.a, t_child.b) // 3, 2
```

This design is inspired by Lua and can be used to simulate some OOP features,
like method overriding.

## Operator overloads

Many snap operators can be overloaded to perform different actions.
The overloading methods must exist somewhere up in the parent object hierarchy.
Eg -

```js
const Point = {
  init(x, y) {
    const t_point = {x: x, y: y}
    return setparent(t_point, this)
  }
}

const a = Point:init(10, 10)
const b = Point:init(3, 4)

let c = a + b // ERROR: Cannot use operator '+' on types 'object' and 'object'.
````
The above code will throw an error since we can't add two tables.
To remedy the error, we introduce a metamethod that overloads the `+` operator:

```js
fn Point.__add(a, b) {
  return Point:init(a.x + b.x, a.y + b.y);
}

// and now if we try again:

const c = a + b;
print(c.x, c.y) // 13, 14
```

A table representing all the overloadable operators
and the method names is listed below for reference:

| Operator    | Method    | Arity |
| :---------- | :---------|:------|
| +           | __add     | 2     |
| +  (unary)  | __unp     | 1     |
| -           | __sub     | 2     |
| -  (unary)  | __unm     | 1     |
| /           | __div     | 2     |
| %           | __mod     | 2     |
| **          | __exp     | 2     |
| >           | __gt      | 2     |
| <           | __lt      | 2     |
| >=          | __gte     | 2     |
| <=          | __lte     | 2     |
| !           | __not     | 1     |
| ^           | __xor     | 2     |
| &           | __band    | 2     |
| |           | __bor     | 2     |
| <<          | __bsl     | 2     |
| >>          | __bsr     | 2     |
| ~           | __bnot    | 1     |
| .           | __indx    | 2     |
| ==          | __eq      | 2     |
| !=          | __neq     | 2     |
| ..          | __concat  | 2     |
| ()          | __call    | 1+    |
| (tostring)  | __tostr   | 1     |


## Classes

Snap provides language level support for OOP, however the same can really just
be simulated using parent tables.

```js
class Point {
  new(x, y) {
    this.x = x
    this.y = y
  }

  __add(a, b) {
    return Point(a.x + b.x, a.y + b.y)
  }

  __tostr() {
    return '(' .. this.x .. ',' .. this.y .. ')'
  }
}

const a = Point(2, 3)
const b = Point(5, 10)

const c = a + b
print(c). // (7, 13)
```
