# The Vyse language specification.

Vyse is a relatively fast, dynamically typed scripting language intended for
embedding in C++ applications, like game engines.

## Operators

Vyse supports the following operators, from highest to lowest precedence:
```
+----------+-------------------------------+---------------+
| Operator | Name/Description              | Associativity |
+----------+-------------------------------+---------------+
| ()       | Call                          | Left-to-right |
| []       | Computed Member access        |               |
| .        | Member access                 |               |
+----------+-------------------------------+---------------+
| +-       | Unary Plus/Minus              |               |
| !~       | Logical Not, Binary Not       |               |
| typeof   | Type of                       |               |
| #        | Length                        |               |
+----------+-------------------------------+---------------+
| **       | Exponent                      | Left-to-right |
+__________+_______________________________+_______________+
| * / %    | Mult / Div / Mod              | Left-to-right |
+----------+-------------------------------+---------------+
| + -      | Add / Sub                     | Left-to-right |
+----------+-------------------------------+---------------+
| << >>    | BitShift left, Bitshift right | Left-to-right |
+----------+-------------------------------+---------------+
| < <=     | Comparison                    | Left-to-right |
| > >=     | Greater, Less                 |               |
|          | Less Equal, Greater Equal     |               |
+----------+-------------------------------+---------------+
| == !=    | Equals, NotEquals             | Left-to-right |
+----------+-------------------------------+---------------+
| &        | Bit And                       | Left-to-right |
+----------+-------------------------------+---------------+
| |        | Bit Xor                       | Left-to-right |
+----------+-------------------------------+---------------+
| |        | Bit Or                        | Left-to-right |
+----------+-------------------------------+---------------+
| &&       | Logic-and                     | Left-to-right |
+----------+-------------------------------+---------------+
| ||       | Logic-or                      | Left-to-right |
+----------+-------------------------------+---------------+
| ?:       | Ternary Operator (misfix)     | Right-to-left |
+----------+-------------------------------+---------------+
| =        | Assignment and compound       | Right-to-left |
| += *=    | assignment operators          |               |
| -= %=    |                               |               |
| **=      |                               |               |
+----------+-------------------------------+---------------+
```

## Variables

Variables behave very similar to javascript. Can be declared with `let` and `const`,
and can hold values of any type.

```js
let mynum = 123; // mutable variables declared with 'let'
mynum = 456; // assignment with `=`.

const fib_base = 1; // variables declared with `const` cannot be mutated
```

## Control Flow.

Vyse supports the following control flow statements:
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

For loops in vyse come with an tiny bit of extra power. The
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

However, Vyse provides a more convienient way to do it when you're
concerned with the value at the `i`th index and not `i` itself:

```lua
for item in my_array {
  print(item);
}
```

## Functions

Functions are declared using the `fn` keyword and like most languages,
called using the `()` operator.

```rs
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

All functions are first class values in Vyse
This means functions can be passed around and used just like
any other value.

Functions are can capture their surrounding environment.
In this sense, all functions are closures.

```rs
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
  race: "cat",
};

print(Tom.name); // Tom
```

Objects can be indexed with any value. Strings, numbers, booleans, functions
and even other tables are all possible key types for an object.
The only exception to this rule is the `nil` value.

```js
print(Tom["age"]); // 19.

let my_obj = { a: 1, b: 2 };
Tom[my_obj] = 123;

print(Tom[my_obj]); // 123
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
  meow() {
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

print(t_child.a, t_child.b); // 3, nil
setproto(t_child, t_parent); // set t_parent as t_child's child
print(t_child.a, t_child.b); // 3, 2
```

This design is inspired by Lua and can be used to simulate some OOP features,
like method overriding.

## Operator overloads

Many vyse operators can be overloaded to perform different actions.
The overloading methods must exist somewhere up in the parent object hierarchy.
Eg -

```js
const Point = {
  init(x, y) {
    const t_point = { x: x, y: y }
    return setproto(t_point, self)
  }
}

const a = Point:init(10, 10)
const b = Point:init(3, 4)

let c = a + b // ERROR: Cannot use operator '+' on types 'object' and 'object'.
```

The above code will throw an error since we can't add two tables.
To remedy the error, we introduce a metamethod that overloads the `+` operator:

```rs
fn Point.__add(a, b) {
  return Point:init(a.x + b.x, a.y + b.y);
}

// and now if we try again:

const c = a + b;
print(c.x, c.y) // 13, 14
```

A table representing all the overloadable operators
and the method names is listed below for reference:

| Operator   | Method     | Arity |
| :--------- | :--------- | :---- |
| +          | \_\_add    | 2     |
| + (unary)  | \_\_unp    | 1     |
| -          | \_\_sub    | 2     |
| - (unary)  | \_\_unm    | 1     |
| /          | \_\_div    | 2     |
| %          | \_\_mod    | 2     |
| \*\*       | \_\_exp    | 2     |
| >          | \_\_gt     | 2     |
| <          | \_\_lt     | 2     |
| >=         | \_\_gte    | 2     |
| <=         | \_\_lte    | 2     |
| !          | \_\_not    | 1     |
| ^          | \_\_xor    | 2     |
| &          | \_\_band   | 2     |
| \|         | \_\_bor    | 2     |
| <<         | \_\_bsl    | 2     |
| >>         | \_\_bsr    | 2     |
| ~          | \_\_bnot   | 1     |
| .          | \_\_indx   | 2     |
| ==         | \_\_eq     | 2     |
| !=         | \_\_neq    | 2     |
| ..         | \_\_concat | 2     |
| ()         | \_\_call   | any   |
| (tostring) | \_\_tostr  | 1     |
