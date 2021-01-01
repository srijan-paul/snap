# Snap

Snap is a dynamically typed, interpreted and fast scriptling lanauge inspired by Lua for rapid prototyping of
applications. (Note that this is a work in progresss project and a lot of the features promised are currently under
development, It si advised to not use snap for any development purposes in it's current stage).

# Table of Contents

- [Goal](#Goal)
- [Overview](#Overview)
- [Roadmap](#Roadmap)
- [Implementation](#Implementation)
- [Building](#Building)
- [Development](#Development)

# Goal

Snap aims to polish the Lua programming language, by adding popularly desired features on top of it, whilst providing
the same speed and minimalism as Lua.
The several complaints that people have about Lua that Snap intends to address are:

- Variables are global by default
- Long keywords like `local` and the use of `do .. end` to designate blocks instead of `{}` makes it unlikeable to
  programmers coming from C.
- The concept of metatables and metamethods is somewhat unintuitive to new programmers.
- Tables being used as both arrays and maps can have unwanted side effects leading to unoptimized code.
- Lacks many features that are common in other languages such as :
  - Compound assignment operators (`+=`, `-=`, `*=`) and decrement and increment operators (`++`, `--`) .
  - `continue` statement for skipping over some loop iterations.
  - `switch` statements.
- Arrays starting at 1.

Snap aims to keep most of what Lua provides, but address the aforementioned complaints and offer the following QoL feautures:

- A Familiar syntax for programmers migrating from Javascript and C
- An easy to integrate C++ API for easy embedding in applications
- Syntactic sugar for OOP that boils down to the very same metatable and metamethod heirarchy.

# Overview

Here is an overview of the language's syntax and features:

```js
// variable declaration and assignment
const PI = 3.141659; // variables declared with const are immutable
let radius = 10;

// Function declaration
fn calculate_area(r) {
	return PI * r ** 2; // '**' operator is used for exponentiation.
}

let area = calculate_area(radius); // function calling

// constructing objects is very similar to Javascript.

let goblin = {
  name: "Bobo",
  health: 12,
	damage: 4,
  attack(target) {
    target.health -= this.damage;
		print("Goblin attacks!");
  },
};

// calling methods is done with the `:` operator.
let wolf = { health: 20 }
goblin:attack(wolf);

// Arrays are intuitive too
let numbers = [1, 3, 5, 7, 9];

// the '#' operator returns length of arrays
print('I have ' .. #numbers .. 'numbers!'); // '..' is the concat operator

// looping is very similar to lua.
for i = 1, #numbers  {
	print(numbers[i]);
}

// conditionals
math.randseed(time.now());
let number = math.random(0, 100);

let guess = int(input()); // read user input and parse as integer.

while true {
	if guess < number {
		print("Too low! Try higher");
	} else if guess > number {
		print("Too hight! Guess lower");
	} else {
		print("You guessed it! The number was " .. number);
		break;
	}
}
```

# Roadmap.

Currently, snap supports basic control flow, variable declaration, usage and basic collection data structures.
To move towards a more complete implementation, the following tasks have to be attended to:

1. Implement optionally NaN boxed values
2. Implement Garbage Collection (Mark sweep).
3. Implement parent tables (metatables in Lua).
4. Add a complete list of collection types. (Arrays and tables)
5. Add an optimization pass over the AST to implement tree folding.
6. Implement proper error handling in all the passes.

# Implementation.

Snap runs on the Snap Virtual Machine (svm for short). The VM has a stack based architechture that operates on 1 byte long opcodes.
User written programs are first compiled into Snap Bytecode over 2 passes and the bytecode is then executed by the snap virtual machine.

The several stages involved are :

### Lexing / Tokenizing (String -> Token)

The snap lexer resides in the `src/syntax/scanner.hpp` file, A simple hand written lexer that accepts a string returns a
token whenever the method `next_token()` is called.
The Lexer is called from within the parser, but can also be instantiated and used stand-alone for testing purposes.

### Parsing (Tokens -> AST)

The Parser transforms an incoming stream of tokens into an Abstract Syntax Tree, which is then handed over to the Bytecode Compiler.
The `src/syntax/parser.cpp` implementation employs recursive descent parsing to parse both expressions and statements.

### Compiling (AST -> Bytecode)

The Compiler transforms an AST produced by the parser into Bytecode instructions in the format specified by the SVM.
The compiler directly writes into chunks of code called `Block`. The Block in question is owned by the VM, that the compiler
has write access to.

### SVM

The design of the SVM is very similar to the Lua Virtual Machine despite the fact that it's Stack based as opposed to LuaVM's
register based design (after version 5.1). It consists of a value stack, and a block containing bytecode that the
VM reads from.
CallFrames, Upvalues and Garbage Collection have not been implemented yet, but are a part of the design.

# Building

To build snap from source, it is recommended that you use CMake (version 3.13 or higher).
The build tool used here is Ninja, but you can use any other build tool of your preference (eg- make).

After downloading/cloning snap into a directory, `cd` into it and run the following commands to run the tests:

```
mkdir bin
cd bin
cmake -G .. Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -CMAKE_C_COMPILER=clang -CMAKE_CXX_COMPILER=clang++
ninja
./snap
```

Note that `-CMAKE_C_COMPILER=clang -CMAKE_CXX_COMPILER=clang++` are optional, and you can use any C++ compiler toolchain of your liking.

# Development

If you're looking to fork contribute to snap, It is recommended to have clang-format for formatting and clangd language server for
your text editor. On VSCode, the `C/C++` extension can be used to debug the executable.

All the source and header files for snap are present in the `src` directory in the project's root folder.
The `main.cpp` file currently contains manually written tests for the different modules involved in the project.
