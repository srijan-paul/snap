local s = os.clock()

local function run_test()

	local fib

	fib = function(n)
		if n <= 1 then
			return 1
		end
		return fib(n - 1) + fib(n - 2)
	end

	for _ = 1, 10000 do
		fib(10)
	end
end

run_test()

local e = os.clock()

-- doesn't work because of low precision.
print(1000 * ((e - s) / 10000))
