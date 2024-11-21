fn @main() -> i64 {
  mut x: i64 = 5
  until x == 10 {
    x = x + 1
  }
  ret x
}
