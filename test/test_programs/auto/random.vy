fn gcd(x, y) {
	if x == 0 {
		return y
	}
	return gcd(y % x, x)
}
assert(gcd(100, 2) == 2)

fn is_prime(x) {
	if x < 2 {
		return false
	}
	for i = 2, x {
		if x % i == 0 {
			return false
		}
	}
	return true
}

assert(is_prime(2))
assert(is_prime(3))
assert(is_prime(5))
assert(!is_prime(70))

fn is_palindrome(str) {
	for i = 0, #str {
		if str[i] != str[#str - i - 1] {
			return false
		}
	}
	return true
}
assert(is_palindrome("racecar"))
assert(!is_palindrome("xewd"))

fn is_pythagorean_triplet(x, y, z) {
	return x ** 2 + y ** 2 == z ** 2
}

assert(is_pythagorean_triplet(3, 4, 5))
assert(!is_pythagorean_triplet(3, 4, 12))

fn newton_sqrt(x) {
	let y = x / 2
	const delta = 0.1;
	while y * y - x > delta {
		y = (y + x / y) / 2
	}
	return y
}

const math = import('math')
assert(math.floor(newton_sqrt(100)) == 10)


fn recursive_sum(arr) {
	if #arr == 0 {
		return 0
	} else if #arr == 1 {
		return arr[0]
	}
	return arr[0] + recursive_sum(arr:slice(1, #arr - 1))
}

assert(recursive_sum([1, 2, 3, 4, 5]) == 15)
