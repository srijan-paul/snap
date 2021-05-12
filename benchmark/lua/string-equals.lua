function perftest()
    local i = 0
    local count = 0
    while i < 1000000 do
        if "abc" == "abc" then
            count = count + 1
        end
        if "a slightly longer string" ==
                "a slightly longer string" then
            count = count + 1
        end
        if "a significantly longer string but still not overwhelmingly long string" ==
                "a significantly longer string but still not overwhelmingly long string" then
            count = count + 1
        end
        if "" == "abc" then
            count = count + 1
        end
        if "abc" == "abcd" then
            count = count + 1
        end
        if "changed one character" == "changed !ne character" then
            count = count + 1
        end
        if "a slightly longer string" ==
                "a slightly longer string!" then
            count = count + 1
        end
        if "a slightly longer string" ==
                "a slightly longer strinh" then
            count = count + 1
        end
        if "a significantly longer string but still not overwhelmingly long string" ==
                "another" then
            count = count + 1
        end
        i = i + 1
    end
    print(count)
end

perftest()