#!/usr/bin/env bats

@test "addition using bc" {
  result="$(echo 2+1 | bc)"
  [ "$result" -eq 3 ]
}


@test "addition using dc" {
  result="$(echo 2 2+p | dc)"
  [ "$result" -eq 4 ]
}

@test "subtraction using bc" {
  result="$(echo 2-1 | bc)"
  [ "$result" -eq 1 ]
}

@test "mutiplication using bc" {
  result="$(echo 2*3 | bc)"
  [ "$result" -eq 6 ]
}