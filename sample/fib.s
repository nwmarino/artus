fn @main() -> i64 {
entry:
  mut x: i64[3] = [0, 1, 2]
  fix y: i64 = x[3]
  ret y
}
