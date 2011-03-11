import multiprocessing
import os
import subprocess

HEADERS = Glob( 'src/*.h' )
SOURCES = Glob( 'src/*.cc' )
BINARY = 'digbuild'
LIBRARY_DEPENDENCIES = [ 'sdl', 'SDL_image', 'gl', 'glew', 'glu', 'CEGUI-OPENGL' ]
HEADER_DEPENDENCIES = [ 'boost/shared_ptr.hpp', 'gmtl/gmtl.h' ]
INCLUDE_DIRECTORY_NAMES = [ 'boost_include_dir', 'gmtl_include_dir' ]

def CheckPackageConfig( context, library ):
    context.Message( 'Checking for library %s...' % library )
    if subprocess.call( [ 'pkg-config', '--exists', library ] ) == 0:
        context.Result( 'ok' )
        return True
    else:
        context.Result( 'failed' )
        return False

AddOption(
    '--boost-include-dir',
    dest = 'boost_include_dir',
    nargs = 1,
    type = 'string',
    action = 'store',
    metavar = 'DIR',
    default = '/usr/include',
    help = 'Boost header file directory'
)

AddOption(
    '--gmtl-include-dir',
    dest = 'gmtl_include_dir',
    nargs = 1,
    type = 'string',
    action = 'store',
    metavar = 'DIR',
    default = '/usr/include/gmtl-0.5.2',
    help = 'GMTL header file directory'
)

OPTIMIZE_BINARY = ARGUMENTS.get( 'optimize', 1 )
INCLUDE_ASSERTIONS = not ARGUMENTS.get( 'assert', 0 )
RELEASE_BUILD = ARGUMENTS.get( 'release', 0 )
PROFILE_BINARY = ARGUMENTS.get( 'profile', 0 )

env = Environment()
env.SetOption( 'num_jobs', multiprocessing.cpu_count() * 2 + 1 )

for variable in [ 'PATH', 'TERM', 'HOME', 'DISPLAY' ]:
    env.Append( ENV = { variable : os.environ[variable] } )

env.Append( CCFLAGS = [ 
    '-Wall',
    '-W',
    '-Wshadow',
    '-Wpointer-arith',
    '-Wcast-qual',
    '-Wwrite-strings',
    '-Wredundant-decls',
    '-Wno-unused',
    '-Wno-deprecated'
] )

for name in INCLUDE_DIRECTORY_NAMES:
    directory = GetOption( name )
    env.Append( CPPPATH = [ directory ] )

if OPTIMIZE_BINARY:
    env.Append( CCFLAGS = [ '-O3', '-ffast-math', '-fassociative-math' ] )

if INCLUDE_ASSERTIONS:
    env.Append( CCFLAGS = [ '-DNDEBUG' ] )

if RELEASE_BUILD:
    env.Append( LINKFLAGS = [ '-Wl,--strip-all' ] )
else:
    env.Append( CCFLAGS = [ '-g' ] )

if PROFILE_BINARY:
    env.Append( CCFLAGS = [ '-pg' ] )
    env.Append( LINKFLAGS = [ '-pg' ] )

conf = Configure( env, custom_tests = { 'CheckPackageConfig' : CheckPackageConfig } )

for header in HEADER_DEPENDENCIES:
    if not conf.CheckCXXHeader( header ):
        print 'Required header file %s could not be found.  Aborting.' % header
        Exit( 1 )

for library in LIBRARY_DEPENDENCIES:
    if not conf.CheckPackageConfig( library ):
        print '*** Required library %s could not be found.  Aborting.' % library
        Exit( 1 )

env = conf.Finish()

for library in LIBRARY_DEPENDENCIES:
    env.MergeFlags( [ '!pkg-config %s --cflags --libs' % library ] )

for path in env['CPPPATH']:
    env.Append( CCFLAGS = [ '-isystem%s' % path ] ) 

env.Program( source = SOURCES, target = BINARY )
env.Command( 'src/tags', SOURCES + HEADERS, 'ctags -o $TARGET $SOURCES' )

env.Command( 'prof', BINARY, './%(binary)s && gprof %(binary)s > prof' % { 'binary' : BINARY } )
env.Clean( 'prof', [ 'prof', 'gmon.out' ] )
env.AlwaysBuild( 'prof' )

env.Command( 'run', BINARY, './' + BINARY )
env.AlwaysBuild( 'run' )

env.Default( [ BINARY, 'src/tags' ] )
