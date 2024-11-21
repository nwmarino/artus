fn @foo(mut a: #i64) -> i64 {
entry:
  #a = 1
  ret 0
}

fn @main() -> i64 {
entry:
  fix x: i64 = 1
  mut y: #i64 = &x
  @foo(y)
  fix z: i64 = #y
  ret z
}
