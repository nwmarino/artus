fn @main() -> i64 {
  mut x: i64 = 0
  mut y: i64 = 0

  while x < 10 {
    if x == 7 {
      break
    }

    x += 1

    if x == 5 {
      continue
    }

    y += 1
  }

  ret x
}
