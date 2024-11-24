struct A {
  mut val: i64,
}

fn @main() -> i64 {
  mut a: A = A {
    val: 1,
  }

  a.val = 0
  ret a.val
}
