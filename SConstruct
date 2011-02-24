import os, glob

SetOption( 'num_jobs', 9 ) # Set this to the number of processors you have.  TODO: Automate this.

sources = glob.glob( '*.cc' )
headers = glob.glob( '*.h' )

env = Environment()
env.Append( ENV = {'PATH':os.environ['PATH'], 'TERM':os.environ['TERM'], 'HOME':os.environ['HOME']} ) # Environment variables required by colorgcc.
# TODO: Use -DNDEBUG for production.
#env.Append( CCFLAGS = [ '-isystem/usr/include/gmtl-0.5.2', '-g', '-pg', '-DNDEBUG', '-O3', '-Wall', '-W', '-Wshadow', '-Wpointer-arith', '-Wcast-qual', '-Wwrite-strings', '-Wconversion', '-Wredundant-decls', '-Wno-unused', '-Wno-deprecated' ] )
env.Append( CCFLAGS = [ '-isystem/usr/include/gmtl-0.5.2', '-g', '-DNDEBUG', '-O3', '-Wall', '-W', '-Wshadow', '-Wpointer-arith', '-Wcast-qual', '-Wwrite-strings', '-Wconversion', '-Wredundant-decls', '-Wno-unused', '-Wno-deprecated' ] )
#env.Append( LINKFLAGS = [ '-pg' ] )
env.Append( LINKFLAGS = [] )
env.Append( LIBS = [ 'SDL', 'GL', 'GLU' ] )

# scons && ./digbuild && gprof digbuild > prof

env.Program( source = sources, target = 'digbuild' )

env.Command( 'tags', sources + headers, 'ctags -o $TARGET $SOURCES' )