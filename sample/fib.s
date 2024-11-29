import foo
import far

fn @main() -> i64 {
  fix x: MyStruct = MyStruct { val: 0 }
  ret x.val
}
