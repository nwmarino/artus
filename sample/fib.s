struct A {
  val: i64,
}

fn @main() -> i64 {
  mut x: str = "hello"
  ret x[9]
}
