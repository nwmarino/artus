fn @fib() -> i32 {
entry:
  ret 0
}

fn @main() -> i64 {
entry:
  ret @fib
}
