#!/usr/bin/env bats

setup_file() {
  :
}

setup() {
    pwd
    load 'test_helper/bats-support/load'
    load 'test_helper/bats-assert/load'
    load 'test_helper/bats-file/load'
    DIR="$(pwd)"
    PATH="$DIR/../../src/DynamicModule:$PATH"
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
  assert_dir_exist $DIR/../../bin
  assert_file_exist $DIR/../../nginx-1.21.1.tar.gz
  assert_dir_exist $DIR/../../nginx-1.21.1
  assert_dir_exist $DIR/../../nginx-1.21.1/moduleSrc
}
