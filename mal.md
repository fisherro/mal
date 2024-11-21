A description of the mal language being made as I work through the steps.

# Types and their syntax

Lists are enclosed in parentheses.

    (this is a list)

Vectors are enclosed in brackets.

    [this is a vector]

(What's the difference?)

Maps are enclosed in braces. Odd elements are keys and even elements are their values.

    {"one" 1 "two" 2 "three" 3}

Numbers are (at step 4) integers. Numbers can be added, subtracted, multiplied, and divided.

The atoms `nil`, `false`, and `true` are distinct types.

# Special forms

`def!` adds values for symbols to the environment.

    (def! x 42)
    (/ x 7)

`let*` creates a nested environment.

    (let* (x 42 y 7)
          (/ x y))

`do` evaluates its parameters and returns the values of the last one.

    (do (def! x 24)
        (def! y 7)
        (/ x y))

`if` is a basic conditional.

    (if condition consequent alternate)

`fn*` creates a closure. It is followed by a list of parameters and then the remainder are expressions to be evaluated.

    (fn* (x y)
         (+ (* x x)
            (* y y)))

# Debugging

If `DEBUG-EVAL` has a value other than `nil` or `false`, debug information about evaluation will be printed.

    (do (def! DEBUG-EVAL true)
        (* 5 5))

(I've also added `DEBUG-EVAL-ENV` which, when `DEBUG-EVAL` is true, will cause `eval` to dump its environment as well.)

# Core functions

* `prn` print a single argument
* `list`
* `list?`
* `empty?`
* `count` Returns zero if its argument is `nil`.
* `=`

And the integer operators: + - * / < <= > >=

