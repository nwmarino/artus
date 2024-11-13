fn @fib() -> i64 {
entry:
  ret i64 0
}

fn @main() -> i64 {
entry:
  ret i8 @fib
}
