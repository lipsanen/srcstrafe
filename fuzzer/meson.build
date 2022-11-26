fuzzer_files = ['main.cpp']
fuzzer = executable('fuzzer', fuzzer_files, link_with : [srcstrafe], include_directories : [srcstrafe_all],
  link_args : '-fsanitize=fuzzer')

tester_files = ['main.cpp', 'tester_main.cpp']
tester = executable('tester', tester_files, link_with : [srcstrafe], include_directories : [srcstrafe_all],
  link_args : '-fsanitize=fuzzer-no-link')

