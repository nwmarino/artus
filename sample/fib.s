import foo

fn @zoo(mut a: MyStruct) -> MyStruct {
  a.val = 0
  ret a  
}

fn @zee() -> MyStruct {
  mut x: MyStruct = MyStruct { val: 42 }
  ret @zoo(x)
}

fn @main() -> i64 {
  fix x: MyStruct = @zee
  ret x.val
}
