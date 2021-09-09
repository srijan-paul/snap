 -- '#' operator returns the length of strings.
let str = 'xxx'
let len = #str
str = str..str
len += #str

-- expect 9
return len
