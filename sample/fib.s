fn @foo(mut a: i64, b: i64) -> i64 {
  a = a + 1
  ret a + b
}

fn @main() -> i64 {
entry:
  fix x: i64 = 1
  ret @foo(x, 1)
}
