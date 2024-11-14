fn @foo() -> char {
entry:
  ret char 'a'
}

fn @main() -> i64 {
entry:
  ret @foo
}
