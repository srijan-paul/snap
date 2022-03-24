assert('abc'[0] == 'a')
assert('13':to_num() + 12 == 25)

const s = "-ab--ab--";
assert(s:replace("ab", "pqr") == "-pqr--pqr--")
assert(s:replace("pqr", "ab") == "-ab--ab--")
assert(s:replace('-', 'x') == 'xabxxabxx')
assert('xbox':replace('box', '') == 'x')
assert('box':replace('box', '') == '')
assert('xox':replace('x', '') == 'o')
assert('xxxx':replace('x', 'x') == 'xxxx')

assert('1':islower() == false)
assert('0123456789':isdigit() == true)
assert('0123456789'[2]:isdigit() == true)
assert('A':islower() == false)
assert('aabcas':islower() == true)
assert('Axxxx':islower() == false)
assert('xxxxA':islower() == false)
