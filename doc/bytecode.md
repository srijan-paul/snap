# Snap bytecode reference.

This document contains all relevant information about the Snap VM's bytecode.
Snap's VM is a stack based virtual machine. In this document, we will use the following terms
quite often:


| Term      | Meaning                                                                                                               |
| --------- | --------------------------------------------------------------------------------------------------------------------- |
| TOS       | Top Of Stack. The location of the topmost value in the VM's stack.                                                    |
| POP()     | Discard the value on top of the stack and  return it.                                                                 |
| PUSH(x)   | Push `x` on top of the VM's stack.                                                                                    |
| CONSTANTS | The constant pool of the current function. It is an array containing the literal values used in the current function. |
| BASE      | Base index of the current callframe. This is used to read local variables                                             |


## Snap Bytecode

| Opcode              | Operands           | Arity         | Stack Effect         | Stack state                               | Description                                                  |
| ------------------- | ------------------ | ------------- | -------------------- | ----------------------------------------- | ------------------------------------------------------------ |
| load_const          | Idx                | 1             | 1                    | [] -> [CONSTANTS[Idx]]                    | Loads a value from the pool onto the stack.                  |
| get_global          | NameIdx            | 1             | 1                    | [] -> [Globals[CONSTANTS[Idx]]            | Pushes the global with name CONSTANTS[Idx] onto the stack    |
| set_global          | NameIdx            | 1             | -1                   | [Value] -> []                             | Pops a value off the stack and sets the global with name Constants[NameIdx] to it's value |
| table_get           | KeyIdx             | 1             | 0                    | [Table] -> [Table.get(CONSTANTS[KeyIdx])] |                                                              |
| table_set           | KeyIdx             | 1             | -1                   | [Table, Value] ->[Value]                  |                                                              |
| table_get_no_popo   | KeyIdx             | 1             | 1                    | [Table] -> [Table, Value]                 | Key = CONSTANTS[KeyIdx]; push(Table.get(key))                |
| set_var             | Idx                | 1             | -1                   | [Value] -> []                             | STACK[BASE + Idx] = POP()                                    |
| get_var             | Idx                | 1             | 1                    | [] -> [STACK[BASE + Idx]]                 |                                                              |
| set_upval           | Idx                | 1             | -1                   | [Value] -> []                             | UPVALUES[Idx] = POP()                                        |
| get_upval           | Idx                | 1             | 1                    | [] -> [UPVALUES[Idx]]                     |                                                              |
| make_func           | Numupvals, ...rest | Numupvals + 1 | 1                    | [] -> [Function]                          |                                                              |
| prep_method_call    | KeyIdx             | 1             | 1                    | [Table] -> [Table.get(Idx), Table]        | table = TOS; TOS = Table.get(CONSTANTS[KeyIdx]); PUSH(Table); |
| call_func           | NumArgs            | 1             | 0                    | /* New CallFrame */                       | Calls the function object present at a stack depth of NumArgs + 1, every value above that is treated as an argument to the function |
| pop                 |                    | 0             | -1                   | [Value] -> []                             | POP();                                                       |
| add                 |                    | 0             | -1                   | [A, B] -> [A + B]                         | A = POP(); B = POP(); PUSH(A + B);                           |
| concat              |                    | 0             | -1                   | [A, B] -> [A..B]                          |                                                              |
| sub                 |                    | 0             | -1                   | [A, B] -> [A+B]                           |                                                              |
| mult                |                    | 0             | -1                   | [A, B] -> [A * B]                         |                                                              |
| div                 |                    | 0             | -1                   | [A, B] -> [A / B]                         |                                                              |
| eq                  |                    | 0             | -1                   | [A, B] -> [A==B]                          |                                                              |
| neq                 |                    | 0             | -1                   | [A, B] -> [A != B]                        |                                                              |
| lshift              |                    | 0             | -1                   | [A, B] -> [A << B]                        |                                                              |
| rshift              |                    | 0             | -1                   | [A, B] -> [A >> B]                        |                                                              |
| band                |                    | 0             | -1                   | [A, B] -> [A&B]                           |                                                              |
| bor                 |                    | 0             | -1                   | [A, B] -> [A\|B]                          |                                                              |
| gt                  |                    | 0             | -1                   | [A,B] ->[A>B]                             |                                                              |
| lt                  |                    | 0             | -1                   | [A,B]->[A<B]                              |                                                              |
| gte                 |                    | 0             | -1                   | [A,B]->[A>=B]                             |                                                              |
| lte                 |                    | 0             | -1                   | [A,B]->[A<=B]                             |                                                              |
| negate              |                    | 0             | 0                    | [A]->[-A]                                 |                                                              |
| lnot                |                    | 0             | 0                    | [A]->[!A]                                 |                                                              |
| load_nil            |                    | 0             | 1                    | [] -> [nil]                               |                                                              |
| close_upval         |                    | 0             | -1                   | [Value]->[]                               | Closes the most recently captured upvalue by moving it to the heap |
| return_val          |                    | 0             | (Pops callframe)     |                                           | Pops everything from the current stack top all way to the base of the current CallFrame, then pushes the return value on top. |
| new_table           |                    | 0             | 1                    | [] -> [Table]                             | Creates a new table and pushes it on top of the stack.       |
| index_set           |                    | 0             | -2                   | [Table, Key, Value] -> [Value]            | Sets Table[key] to Value. here key is always a computed index. Value = POP(); Key = POP(); Table = POP(); Table.set(Key, Value); PUSH(Value); |
| table_add_field     |                    | 0             | -2                   | [Table, Field, Value] -> [Table]          | Sets Table.Field = Value. Doesn't pop the table off the stack. This instruction is used when creating tables at runtime from their source description. |
| index               |                    | 0             | -1                   | [Table, Key] -> [Table[Key]]              | Key = POP(); Table = POP(); PUSH(Table.get(Key));            |
| index_no_pop        |                    | 0             | 1                    | [Table, Key] -> [Table, Key, Table[Key]]  | Same as index, but doesn't pop the table or key off the stack. Used for compound assignment operators like `+=` |
| jmp                 | A, B               | 2             | 0                    |                                           | Reads the next two bytes of opcodes, stitches them together and jumps forward in the instruction stream. A = FETCH(); B = FETCH(); IP += (A << 8) \| B |
| jmp_back            |                    | 2             | 0                    |                                           | Same as `jmp`, but decrements the `IP` instead. A = FETCH(); B = FETCH(); IP = IP - (A<<8)\|B |
| jmp_if_false_or_pop |                    | 2             | -1 (if TOS is false) |                                           | If the value on top of the stack is falsy, then jump forward `(A<<8)|B` instructions. Otherwise pop the value and continue. |
| jmp_if_true_or_pop  |                    | 2             | -1 (if TOS is true)  |                                           | If the value on top of the stack is truthy, then jump forward `(A<<8)|B` instructions. Otherwise pop the value and continue. |
| pop_jmp_if_false    |                    | 2             | -1                   | [Value] -> []                             | Pop a value off the stack. If the Popped value is falsy, then jump forward `(A<<8)|B` bytes. Otherwise continue. |
