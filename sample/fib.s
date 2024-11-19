fn @main() -> i64 {
entry:
  mut numbers: i64[3] = [0, 1, 2]
  numbers[1] = 0
  ret numbers[1]
}
