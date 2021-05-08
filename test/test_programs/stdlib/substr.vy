fn delete_char(s, index) {
  return String.substr(s, 0, index) .. s:substr(index + 1)
}

-- expect: Moon
return delete_char('Moron', 2) 