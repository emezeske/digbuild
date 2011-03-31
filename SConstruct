import multiprocessing
import os
import subprocess

HEADERS = Glob( 'src/*.h' )
SOURCES = Glob( 'src/*.cc' )
BINARY = 'digbuild'
PACKAGE_DEPENDENCIES = [ 'agar', 'gl', 'glew', 'glu', 'sdl', 'SDL_image' ]
LIBRARY_DEPENDENCIES = [ 'boost_thread' ]
HEADER_DEPENDENCIES = [ 'boost/shared_ptr.hpp', 'gmtl/gmtl.h', 'boost/threadpool.hpp' ]
INCLUDE_DIRECTORY_NAMES = [ 'boost_include_dir', 'gmtl_include_dir' ]

def CheckPackageConfig( context, library ):
    context.Message( 'Checking for library %s...' % library )
    command = None

    with open( '/dev/null', 'w' ) as null:
        if subprocess.call( [ 'pkg-config', '--exists', library ], stdout = null ) == 0:
            command = [ 'pkg-config', library, '--cflags', '--libs' ]
        elif subprocess.call( [ 'which', '%s-config' % library ], stdout = null ) == 0:
            command = [ '%s-config' % library, '--cflags', '--libs' ]

    if command is not None:
        context.Result( 'ok' )
        flags = subprocess.Popen( command, stdout = subprocess.PIPE ).communicate()[0]
        return True, flags
    else:
        context.Result( 'failed' )
        return False, ''

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

OPTIMIZE_BINARY = int( ARGUMENTS.get( 'optimize', 1 ) )
INCLUDE_ASSERTIONS = not int( ARGUMENTS.get( 'assert', 0 ) )
RELEASE_BUILD = int( ARGUMENTS.get( 'release', 0 ) )
PROFILE_BINARY = int( ARGUMENTS.get( 'profile', 0 ) )

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

for library in PACKAGE_DEPENDENCIES:
    success, flags = conf.CheckPackageConfig( library )
    if success:
        env.MergeFlags( flags )
    else:
        print 'Required library %s could not be found.  Aborting.' % library
        Exit( 1 )

for library in LIBRARY_DEPENDENCIES:
    if conf.CheckLib( library ):
        env.Append( LINKFLAGS = [ '-l' + library ] )
    else:
        print 'Required library %s could not be found.  Aborting.' % library
        Exit( 1 )

env = conf.Finish()

for path in env['CPPPATH']:
    env.Append( CCFLAGS = [ '-isystem%s' % path ] ) 

env.Program( source = SOURCES, target = BINARY )
env.Command( 'tags', SOURCES + HEADERS, 'ctags -o $TARGET $SOURCES' )

env.Command( 'prof', BINARY, './%(binary)s && gprof %(binary)s > prof' % { 'binary' : BINARY } )
env.Clean( 'prof', [ 'prof', 'gmon.out' ] )
env.AlwaysBuild( 'prof' )

env.Command( 'run', BINARY, './' + BINARY )
env.AlwaysBuild( 'run' )

env.Default( [ BINARY, 'tags' ] )
