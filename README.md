# artus

## Types
| Identifier | Name | Example |
|------------|------|---------|
| `bool` | boolean | `0`, `1`
| `char` | char/8-bit | `'a'`, `256`
| `i32` | 32-bit signed | `2^31`
| `i64` | 64-bit signed | `2^63`
| `u8` | 8-bit unsigned | `2^8`
| `u32` | 32-bit unsigned | `2^32`
| `u64` | 64-bit unsigned | `2^64`
| `str` | string | `"abc"`
| `f64` | floating point | `3.14`

## Operators
| Operator | Precedence | Use |
|----------|------------|-----|
| `=` `+=` `-=` `*=` `/=` | 1 | Assignment
| `&&` `\|\|` `^^` | 2 | Logical Comparison (and, or, xor)
| `==` `!=` | 3 | Equality Comparison
| `<` `>` `<=` `>=` | 4 | Inequalities
| `+` `-` | 5 | Additive Ops
| `*` `/` | 6 | Multiplicative Ops
| `#` `&` `!` `-` | 7 | Unary Ops

## Pointers
```go
/// ptr address-of
mut x: i64 = 5
mut y: #i64 = &x

/// ptr dereference
#y = 0
```

## Arrays
```go
// array initialization
mut nums: i64[3] = [0, 1, 2]

// array access
fix a: i64 = nums[0]
```

## Functions
```
function-decl: 
        'fn' '@' <identifier> '(' [param-list] ')' '->' [type] <stmt>
      | 'fn' '@' <identifier> '(' [param-list] ')' <stmt>
      | 'fn' '@' <identifier> '->' [type] <stmt>
      | 'fn' '@' <identifier> <stmt>

param-list: 
        param [',' param-list]

param:
        'mut' <identifier> ':' [type]
      | <identifier> ':' [type]

type:   <identifier>
```
```go
// main
fn @main() -> i64 { entry: ... }

// otherwise
fn @foo(a: i64, mut b: i64[2]) -> i64 { entry: ... }

// function calls, empty:
@foo

// function calls, with arguments
@foo(1, [2, 3])
```

## Variables
```
var-decl:
        'fix' <identifier> ':' [type] '=' <expr>
      | 'mut' <identifier> ':' [type]
      | 'mut' <identifier> ':' [type] '=' <expr>

type:   <identifier>
```
```go
// immutable assignment
fix <identifier>: <type> = <expr>

// mutable assignment
mut <identifier>: <type> = <expr>
```

## Statements
```go
// return stmt
ret <expr>
```

## Control Flow

### If Statements
```
if-stmt:
        'if' <expr> <stmt>
      | 'if' <expr> <stmt> 'else' <stmt>
```
```go
if cond {
    ...
}

if cond {
    ...
} else {
    ...
}

if cond {
    ...
} else if cond {
    ...
} else {
    ...
}
```

### Loops

#### While Loops
```
while-stmt:
        'while' <expr> <stmt>
```
```go
while cond {
    ...
}
```

#### Until Loops
```
until-stmt:
        'until' <expr> <stmt>
```
```go
until cond {
    ...
}
```

### Match Statements
```
match-stmt:
        'match' <expr> '{' [match-case-list] '}'

match-case-list:
        match-case [',' match-case-list]

match-case:
        case-stmt
      | default-stmt

case-stmt:
        <expr> => <stmt>

default-stmt:
        '_' => <stmt>
```
```go
match <expr> {
    <expr> => ...,
    <expr> => { ... },
    _ => { ... },
}
```
