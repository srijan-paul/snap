from __future__ import print_function

# Map "range" to an efficient range in both Python 2 and 3.
try:
    range = xrange
except NameError:
    pass

list_ = []
for i in range(0, 1000000):
  list_.append(i)

sum_ = 0
for i in list_:
  sum_ += i
print(sum_)