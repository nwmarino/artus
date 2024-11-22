fn @main() -> i64 {
  mut x: i64 = 5
  while x != 80 {
    x *= 2
  }
  x -= 5
  ret x 
}
