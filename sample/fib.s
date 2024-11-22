fn @main() -> i64 {
  mut x: i64 = 5
  match x {
    1 => x = 1,
    2 => x = 1,
  }
  ret x 
}
