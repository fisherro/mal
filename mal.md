A description of the mal language being made as I work through the steps.

Lists are enclosed in parentheses.

    (this is a list)

Vectors are enclosed in brackets.

    [this is a vector]

(What's the difference?)

Maps are enclosed in braces. Odd elements are keys and even elements are their values.

    {"one" 1 "two" 2 "three" 3}

Numbers are (at step 3) integers. Numbers can be added, subtracted, multiplied, and divided.

The def! special form adds values for symbols to the environment.

    (def! x 42)
    (/ x 7)

The let* special form creates a nested environment.

    (let* (x 42 y 7)
          (/ x y))


