fn @foo() -> bool {
entry:
  ret 1
}

fn @main() -> i64 {
entry:
  ret @foo
}
