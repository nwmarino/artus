fn @foo() -> i64 {
  ret 42
}

fn @bar() -> i64 {
  ret @foo
}