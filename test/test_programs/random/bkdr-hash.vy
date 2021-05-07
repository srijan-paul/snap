-- BKDR hash algorithm: https://www.programmingalgorithms.com/algorithm/bkdr-hash/cpp/

fn bkdr_hash(str) {
  const seed = 131
  let hash = 0
  for i = 1, str:len() 
    hash = (hash * seed) + str[i]
  return hash
}

assert(bkdr_hash('jdfgsdhfsdfsd 6445dsfsd7fg/*/+bfjsdgf%$^') == 3255416723)