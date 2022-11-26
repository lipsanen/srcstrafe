gtest_proj = subproject('gtest')
gtest_dep = gtest_proj.get_variable('gtest_dep')

unittest_files = ['main.cpp', 'strafe_utils.cpp']
unittests = executable('unittests', unittest_files, link_with : [srcstrafe], include_directories : [srcstrafe_all], dependencies : [gtest_dep])
test('Unit tests', unittests, workdir : meson.source_root(), timeout: 0)
