import os, sys, subprocess

########################################
# Root compilation settings, shared among all builds

env = Environment()
env.Replace(CXX='g++-9')
env.Append(CPPFLAGS = ['-Wall'])
env.Append(CXXFLAGS = ['-std=c++17', '-O0', '-g'])
env.Append(CPPFLAGS = ['-I./vendor/glad/include'])
env.Append(CPPFLAGS = ['-I./vendor/imgui'])
env.ParseConfig('sdl2-config --cflags --libs')
env['ENV']['TERM'] = os.environ['TERM'] # Color terminal

########################################

third_party_env = env.Clone()
third_party_env.VariantDir('build/vendor', 'vendor', duplicate=0)

glad_lib = third_party_env.StaticLibrary(target='build/glad', source=Glob('build/vendor/glad/src/*.c') )
imgui_lib = third_party_env.StaticLibrary(target='build/imgui', source=Glob('build/vendor/imgui/*.cpp') )

########################################
# qnes

qnes = env.Clone()
qnes.VariantDir('build', 'src', duplicate=0)
qnes.Append(CPPFLAGS = ['-I./src'])

if sys.platform == 'darwin':
  
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

qnes_lib = qnes.StaticLibrary(target='build/libqnes', source=[qnes_src])

qnes.VariantDir('build/app', 'app', duplicate=0)
qnes.Program('build/qnes', source=['build/app/qnes.cpp', qnes_lib, imgui_lib, Glob('vendor/glad/src/*.c')] )