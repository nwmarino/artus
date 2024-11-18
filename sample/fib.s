fn @main() -> i64 {
entry:
  fix x: i64 = 0
  mut y: *i64 = &x

  ret i64 *y
}
