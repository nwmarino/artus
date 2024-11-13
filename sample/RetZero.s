fn @main() -> i64 {
entry:
  mut x: i64
  fix y: i64 = 2
  x = y
  ret x - 2
}
