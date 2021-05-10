------------------
-- Toggle class --
------------------

const Toggle = {
  new(start_state) {
    const o = { state: start_state }
    return setproto(o, self)
  },
  
  value() {
    return self.state
  },

  activate() {
    self.state = !self.state
    return self
  }
}

------------------
--- NthToggle ----
------------------

const NthToggle = Toggle:new()

NthToggle.activate = fn (self) {
  self.counter = self.counter + 1
  if self.counter >= self.count_max {
    Toggle.activate(self)
    self.counter = 0
  }
  return self
}

NthToggle.new = fn (self, start_state, max_counter) {
  const o = Toggle.new(self, start_state)
  o.count_max = max_counter
  o.counter = 0
  return o 
}

fn main() {
  const N = 100000
  let val = true 
  let toggle = Toggle:new(val)

  for i = 0, N {
    val = toggle:activate():value()
    val = toggle:activate():value()
    val = toggle:activate():value()
    val = toggle:activate():value()
    val = toggle:activate():value()
    val = toggle:activate():value()
    val = toggle:activate():value()
    val = toggle:activate():value()
    val = toggle:activate():value()
    val = toggle:activate():value()
  }

  print(val)

  val =  true 
  const ntoggle = NthToggle:new(val, 3)
  for i = 0, N {
    val = ntoggle:activate():value()
    val = ntoggle:activate():value()
    val = ntoggle:activate():value()
    val = ntoggle:activate():value()
    val = ntoggle:activate():value()
    val = ntoggle:activate():value()
    val = ntoggle:activate():value()
    val = ntoggle:activate():value()
    val = ntoggle:activate():value()
    val = ntoggle:activate():value()
  }

  print(val)
}

main()