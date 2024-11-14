# artus

| type id | desc. | example |
|---------|-------|---------|
| `bool`    | boolean | `0`, `1`
| `char`    | char/8-bit | `'a'`, `256`
| `i32`   | 32-bit signed | `2^31`
| `i64`   | 64-bit signed | `2^63`
| `u8`    | 8-bit unsigned | `2^8`
| `u32`   | 32-bit unsigned | `2^32`
| `u64`   | 64-bit unsigned | `2^64`
| `string`   | string | `"abc"`
| `fp64` | floating point | `3.14`

function signature

```rs
fn @main() -> i64 { entry: ... }
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

