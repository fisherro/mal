## Command line options (cpprlf)

If the interpreter is passed command line arguments that begin with `--`, this will add a binding to the REPL environment.

`--FOO` would bind `FOO` to `true`.

`--BAR=false` would bind `BAR` to `false`.

There are some debug values that can be set.

`--DEBUG-EVAL`: As with vanilla `mal`, this will make the interpreter print the AST when `eval` is called.

`--DEBUG-EVAL-ENV`: If `DEBUG-EVAL` is true, this will dump the current environment whenever `eval` is called.

`--DEBUG-EVAL-ENV-FULL`: If `DEBUG-EVAL` is true, this will dump the full chain of environments whenever `eval` is called. (This will take precedence over plain `DEBUG-EVAL-ENV`.)

## Readably

When a function says it prints "readably", that means that strings will be enclosed by double-quotes and double-quotes, backslashes, and newlines will be escaped.

## Reader macros

## Special forms

`(def! <name> <value>)`: Binds a name to a value in the current environment.

`(let* (<name1> <value1> ...) <expr>`: Creates a nested environment, binds the given names to values, and then evaluates `expr`.

`(defmacro! <symbol> <expr>)`: The `expr` must evaluate to a function. The function is then bound as a macro to `symbol` in the current environment.

`(try* <expr1> (catch* <symbol> <expr2>))`: Evaluateds `expr1`. If `expr1` throws an exception, it is bound in a new nested environment to `symbol` and `expr2` is evaluated.

`(do <expr> ...)`: Evaluates the given expressions in order and returns the value of the last one.

`(if <expr1> <expr2> <expr3>)`: If `expr1` evaluates to anything other than `nil` or `false`, evaluates and returns the value of `expr2`. Otherwise, evaluates and returns the result of `expr3`.

`quasiquote`

`fn*`

`quote`

## Core

`= < <= > >= + - * /`: Operators for integers.

`pr-str`: Prints readably its arguments separated by spaces. Returns a string.

`str`: Prints (not readably) its arguments separated by spaces. Returns a string.

`prn`: Prints readably its arguments separated by spaces to stdout. Returns `nil`.

`println`: Prints (not readably) its arguments separated by spaces and followed by a newline to stdout. Returns `nil`.

`list`: Creates a list from its arguments.

`list?`

`empty?`

`count`

`read-string`

`slurp`

`atom`

`atom?`

`deref`

`reset!`

`swap!`

`cons`: this function takes a list as its second parameter and returns a new list that has the first argument prepended to it.

`concat`: this functions takes 0 or more lists as parameters and returns a new list that is a concatenation of all the list parameters.

`nth`

`first`

`rest`

`throw`

`nil?`

`true?`

`false?`

`symbol`

`symbol?`

`keyword`

`keyword?`

`vector`

`vector?`

`hash-map`

`map?`

`assoc`

`dissoc`

`get`: takes a hash-map and a key and returns the value of looking up that key in the hash-map. If the key is not found in the hash-map then nil is returned.

`contain?`

`keys`

`vals`

`sequential?`

`apply`

`map`: takes a function and a list (or vector) and evaluates the function against every element of the list (or vector) one at a time and returns the results as a list.

`readline`

`fn?`

`string?`

`number?`

`conj`: takes a collection and one or more elements as arguments and returns a new collection which includes the original collection and the new elements. If the collection is a list, a new list is returned with the elements inserted at the start of the given list in opposite order; if the collection is a vector, a new vector is returned with the elements added to the end of the given vector.

`seq`: takes a list, vector, string, or nil. If an empty list, empty vector, or empty string ("") is passed in then nil is returned. Otherwise, a list is returned unchanged, a vector is converted into a list, and a string is converted to a list containing the original string split into single character strings.

`with-meta`

`meta`

`time-ms`: takes no arguments and returns the number of milliseconds since epoch (00:00:00 UTC January 1, 1970), or, if not possible, since another point in time (time-ms is usually used relatively to measure time durations). After time-ms is implemented, you can run the performance micro-benchmarks by running make perf^quux.

### Cpprlf specific

`(system <string>)`: Invokes the system shell with the given string and returns an integer result code.

`(backtick <string>)`: Invokes the system shell with the given string, collects the commands stdout output, and returns it as a string.

`(string <string> ...)`: Concatenates the given strings into a single string.

## Predefined

`not`

`cond`

    (cond
        (vector? ast)            (list 'vec (qq-foldr ast))
        (map? ast)               (list 'quote ast)
        (symbol? ast)            (list 'quote ast)
        (not (list? ast))        ast
        (= (first ast) 'unquote) (if (= 2 (count ast))
                                   (nth ast 1)
                                   (throw "unquote expects 1 argument"))
        "else"                   (qq-foldr ast))

eval load-file *ARGV* *host-language*


