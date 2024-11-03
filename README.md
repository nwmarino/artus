# artus

| type id | desc. | example |
|---------|-------|---------|
| `bool`    | boolean | `0`, `1`
| `char`    | char/8-bit | `'a'`, `256`
| `i32`   | 32-bit signed | `2^31`
| `i64`   | 64-bit signed | `2^63`
| `u8`    | 8-bit unsigned |
| `u32`   | 32-bit unsigned |
| `u64`   | 64-bit unsigned |
| `str`   | string | `"abc"`
| `fp64` | floating point | `3.14`

function signature

```rs
fn @main() -> i64 { entry: ... }
```

immutable assignment
```rs
fix <type> <identifier> = <expr>
```

mutable assignment
```rs
mut <type> <identifier> = <expr>
```

statements

```rs
ret <?type> <val>
```

```rs
out <str>
```

```
cin -> <symbol>
```
