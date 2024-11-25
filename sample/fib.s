struct A {
  mut val: i64,
}

enum MyEnum {
  a,
  b,
  c,
}

fn @main() -> i64 {
  mut a: MyEnum = MyEnum::b
  a += 1
  ret a
}
