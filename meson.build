project('demogobbler', ['c', 'cpp'], default_options : ['c_std=c11', 'cpp_std=c++17', 'default_library=both'])

add_project_arguments('-fno-omit-frame-pointer', language : ['c', 'cpp'])
add_project_arguments('-ggdb', language : ['c', 'cpp'])
add_project_arguments('-Werror-implicit-function-declaration', language : ['c'])
add_project_arguments('-Werror=incompatible-pointer-types', language : ['c'])

if get_option('fuzz')
    add_project_arguments('-fsanitize=fuzzer', language : ['c', 'cpp'])
endif

srcstrafe_export = include_directories(['include'])
srcstrafe_all =  include_directories(['include', 'src'])

subdir('src')

if get_option('fuzz')
    subdir('fuzzer')
else
    subdir('test')
    subdir('bench')
endif
