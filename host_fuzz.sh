clang -fsanitize=fuzzer,address -o fuzzer_test grid_common/test_harness.c

timeout -k 60s 60s ./fuzzer_test -timeout=10 -max_len=128
fuzzer_exit_code=$?
if [ $fuzzer_exit_code -eq 124 ]; then
  echo "Fuzzer timed out."
  exit 0
else
  echo "Fuzzer exit code: $fuzzer_exit_code"
  cp $(ls crash-*) input.bin
  ls
  echo "cat:"
  cat input.bin
  echo "xxd:"
  cat $(ls crash-*) | xxd
  exit $fuzzer_exit_code
fi
