boost_dep = dependency('boost', modules: ['filesystem', 'iostreams'])
srcstrafe_gnuplot_files = ['main.cpp']
srcstrafe_gnuplot = executable('srcstrafe_gnuplot', srcstrafe_gnuplot_files, link_with : [srcstrafe], include_directories : [srcstrafe_all], dependencies: [boost_dep])

