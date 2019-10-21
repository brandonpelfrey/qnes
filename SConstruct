import os, sys, subprocess

########################################
# Root compilation settings, shared among all builds

env = Environment()
env.Append(CPPFLAGS = ['-Wall'])
env.Append(CXXFLAGS = ['-std=c++17', '-O0', '-g'])
env['ENV']['TERM'] = os.environ['TERM'] # Color terminal

########################################
# qnes

qnes = env.Clone()
qnes.VariantDir('build', 'src', duplicate=0)
qnes.Append(CPPFLAGS = ['-I./src'])

if sys.platform == 'darwin':
  qnes.Append(LIBPATH=['/usr/local/lib'])
  #qnes.Append(LIBS=['SDL2'])
  qnes.Append(CPPFLAGS=['-I/usr/local/include/'])
  qnes.ParseConfig('sdl2-config --cflags --libs')
  qnes.Append(FRAMEWORKS=' OpenGL')
else:
  print('%s is not a supported platform. Please modify the SConstruct file' % (sys.platform))
  sys.exit(1)

# qnes Source Files
qnes_src = []
for root, dirs, files in os.walk("src"):
  for file in files:
    if file.endswith(".cpp"):
      src_path = os.path.join(root, file)
      build_path = src_path.replace('src/', 'build/', 1)
      qnes_src.append( build_path )

qnes.Program('build/qnes', source=[qnes_src])