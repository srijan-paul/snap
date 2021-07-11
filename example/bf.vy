fn bf(src, input) {
    const memory = List.make(30)
    memory:fill(0)
    let mPtr  = 0
    let inPtr = 0
    let ip    = 0
    let stack = []
        
    let out = ""
    while ip < #src {
        assert(ip >= 0, "Invalid instruction pointer")
        const c = src[ip]
        if        c == '+' {
            memory[mPtr] += 1
        } else if c == '-' {
            memory[mPtr] -= 1
        } else if c == '.' {
            out = out .. String.from_code(memory[mPtr])
        } else if c == 'x' {
           memory[mPtr] = input:code_at(inPtr)
           inPtr += 1
        } else if c == '>' {
            mPtr += 1
            assert(mPtr < #memory, "data pointer out of bounds")
        } else if c == '<' {
            mPtr -= 1
            assert(mPtr >= 0, "data pointer cannot go below 0")
        } else if c == '[' {
           if (memory[mPtr] != 0) {
               stack <<< ip
           } else {
               let bcount = 0
               while true {
                    ip += 1
                    assert(ip < #src, "Missing matching ']'");
                    if src[ip] == ']' {
                        if bcount != 0 { bcount -= 1 }
                        else break
                    } else if src[ip] == '[' {
                        bcount += 1
                    }
               }
           }
        } else if c == ']' {
            ip = stack:pop() - 1
        } else {
            assert(false, "Unsupported operation")
        }
        ip += 1
    }
    
    print(out)
}

bf(input("Enter a brainfuck program:\n"))
