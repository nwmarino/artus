struct A {
  val: i64,
}

fn @main() -> i64 {
  fix x: A = A {
    val: 10,
  }

  mut y: i64[3] = [0, 1, 2]
  ret y[0]
}

fn @foo() -> #A {
  ret null
}
