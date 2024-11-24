struct A {
  val: i64,
}

fn @main() -> i64 {
  mut y: i64[3] = [0, 1, 2]
  y = [1, 2, 3]
  ret y[0]
}

fn @foo() -> #A {
  ret null
}
