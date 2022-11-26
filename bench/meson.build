google_benchmark_proj = subproject('google-benchmark')
google_benchmark_dep = google_benchmark_proj.get_variable('google_benchmark_dep')
google_benchmark_main_dep = google_benchmark_proj.get_variable('google_benchmark_main_dep')

bench_files = ['bench.cpp']
bench = executable('srcstrafe_benchmark', bench_files, link_with : [srcstrafe], include_directories : [srcstrafe_all], dependencies : [google_benchmark_dep, google_benchmark_main_dep])
benchmark('Benchmark', bench, workdir : meson.source_root(), timeout: 0)
