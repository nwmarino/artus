fn @main() -> i64 {
entry:
  mut str input
  cout "name: "
  cin -> input
  out "hello, " + input
  ret i32 0
}
