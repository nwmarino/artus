fn @foo(mut a: *i64) -> i64 {
entry:
  *a = 0
  ret 0
}

fn @main() -> i64 {
entry:
  fix x: i64 = 1
  fix y: *i64 = &x
  @foo(y)
  ret *y
}
