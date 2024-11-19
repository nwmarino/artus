fn @main() -> i64 {
entry:
  mut numbers: i64[3] = [1, 2, 3]
  numbers[1] = 0
  ret numbers[1]
}
