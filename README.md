# artus

| type id | desc. | example |
|---------|-------|---------|
| `bool` | boolean | `0`, `1`
| `char` | char/8-bit | `'a'`, `256`
| `i32` | 32-bit signed | `2^31`
| `i64` | 64-bit signed | `2^63`
| `u8` | 8-bit unsigned | `2^8`
| `u32` | 32-bit unsigned | `2^32`
| `u64` | 64-bit unsigned | `2^64`
| `string` | string | `"abc"`
| `f64` | floating point | `3.14`

## Pointers
```rs
/// ptr dereference
fix x: i64 = 5

/// ptr address-of
fix y: *i64 = &x
```

## Arrays
```rs
// array initialization
mut nums: i64[3] = [0, 1, 2]

// array access
fix a: i64 = nums[0]
```

function signature

```rs
// main
fn @main() -> i64 { entry: ... }

// otherwise
fn @foo(a: i64, mut b: i64[2]) -> i64 { entry: ... }

// function calls, empty:
@main

// function calls, with arguments
@foo(1, [2, 3])
```

variables
```rs
// immutable assignment
fix <identifier>: <type> = <expr>

// mutable assignment
mut <identifier>: <type> = <expr>
```

statements

```rs
ret <val>
```

