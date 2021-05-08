-- fn bkdr_hash(str) {
--   const seed = 131
--   let hash = 0
--   for i = 0, #str {
--     hash = (hash * seed) + byte(str[i])
--   } 
--   return hash
-- }
-- 
-- print(bkdr_hash('1231sdas'))

fn delete_char(s, index) {
  return String.substr(s, 0, index) .. String.substr(s, index + 1)
}

-- expect: Moon
return delete_char('Moron', 2) 