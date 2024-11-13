fn @fib(mut n: i64) -> i64 {
entry:
  n = n + 5
  ret n + 1
}

fn @main() -> i64 {
entry:
  ret i64 1
}
