fn @main() -> i64 {
  mut x: i64 = 1
  if x == 0 {
    x = 2
  } else if x == 1 {
    x = 3
  } else {
    x = 4
  }
  ret x
}
