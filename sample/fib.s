fn @main() -> i64 {
  mut x: i64 = 5
  match x {
    1 => x = 1,
    2 => x = 1,
    _ => {
      mut a: i64 = 1
      mut b: i64 = 1
      mut i: i64 = 3
      while i < x {
        mut c: i64 = a + b
        a = b
        b = c
        i = i + 1
      }
      x = b
    }
  }
  ret x 
}
