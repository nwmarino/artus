import foo

fn @zee() -> MyStruct {
  mut x: MyStruct = MyStruct { val: 42 }
  ret x
}

fn @main() -> i64 {
  fix x: MyStruct = @zee
  ret x.val
}