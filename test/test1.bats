#!/usr/bin/env bats

setup_file() {
  :
}

setup() {
    pwd
    load 'test_helper/bats-support/load'
    load 'test_helper/bats-assert/load'
    load 'test_helper/bats-file/load'

    # get the containing directory of this file
    # use $BATS_TEST_FILENAME instead of ${BASH_SOURCE[0]} or $0,
    # as those will point to the bats executable's location or the preprocessed file respectively
    DIR="$( cd "$( dirname "$BATS_TEST_FILENAME" )" >/dev/null 2>&1 && pwd )"
    # make executables in src/ visible to PATH
    PATH="$DIR/../src/DynamicModule:$PATH"
}


teardown() {
  rm -rf bin
  rm -f *.tar.gz
  rm -rf nginx-1.*
}

teardown_file() {
  :
}

@test "testing build.sh" {
  run build.sh 1.21.1 ngx_mirror
  assert_output --partial 'ngx_mirror'
  assert_output --partial 'nginx-1.21.1'
  assert_file_exist $DIR/../src/DynamicModule/build.sh
  assert_dir_exist $DIR/../bin
  assert_file_exist $DIR/../nginx-1.21.1.tar.gz
  assert_dir_exist $DIR/../nginx-1.21.1
  assert_dir_exist $DIR/../nginx-1.21.1/moduleSrc
}

@test "addition using bc" {
  result="$(echo 2+1 | bc)"
  [ "$result" -eq 3 ]
}


@test "addition using dc" {
  skip
  result="$(echo 2 2ls -l+p | dc)"
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

