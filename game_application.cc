#include <GL/glew.h>

#include "sdl_utilities.h"
#include "game_application.h"

//////////////////////////////////////////////////////////////////////////////////
// Local definitions:
//////////////////////////////////////////////////////////////////////////////////

namespace {

typedef std::pair<Vector3i, ChunkSP> PositionChunkPair;
typedef std::vector<PositionChunkPair> ChunkMapValueV;

bool highest_chunk( const PositionChunkPair& a, const PositionChunkPair& b )
{
    return a.first[1] > b.first[1];
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////
// Function definitions for GameApplication:
//////////////////////////////////////////////////////////////////////////////////

GameApplication::GameApplication( SDL_GL_Window &initializer, const int fps ) :
    SDL_GL_Interface( initializer, fps ),
    camera_( Vector3f( -32.0f, 70.0f, -32.0f ), 0.15f, -25.0f, 225.0f ),
    // generator_( time( NULL ) * 91387 + SDL_GetTicks() * 75181 )
    generator_( 0x58afe359358eafd3 ) // FIXME: Using a constant for performance measurements.
{
    SCOPE_TIMER_BEGIN( "World generation" )

    for ( int x = 0; x < 3; ++x )
    {
        for ( int z = 0; z < 3; ++z )
        {
            const Vector2i position( x * WorldGenerator::REGION_SIZE, z * WorldGenerator::REGION_SIZE );
            ChunkV new_chunks = generator_.generate_region( position );
            
            for ( ChunkV::iterator chunk_it = new_chunks.begin(); chunk_it != new_chunks.end(); ++chunk_it )
            {
                chunk_stitch_into_map( *chunk_it, chunks_ );
            }
        }
    }

    SCOPE_TIMER_END

    SCOPE_TIMER_BEGIN( "Lighting" )

    ChunkMapValueV height_sorted_chunks;

    for ( ChunkMap::iterator chunk_it = chunks_.begin(); chunk_it != chunks_.end(); ++chunk_it )
    {
        height_sorted_chunks.push_back( *chunk_it );
    }

    std::sort( height_sorted_chunks.begin(), height_sorted_chunks.end(), highest_chunk );

    for ( ChunkMapValueV::iterator chunk_it = height_sorted_chunks.begin(); chunk_it != height_sorted_chunks.end(); ++chunk_it )
    {
        chunk_it->second->reset_lighting();
    }

    for ( ChunkMapValueV::iterator chunk_it = height_sorted_chunks.begin(); chunk_it != height_sorted_chunks.end(); ++chunk_it )
    {
        chunk_apply_lighting( *chunk_it->second.get() );
    }

    SCOPE_TIMER_END

    SCOPE_TIMER_BEGIN( "Updating geometry" )

    for ( ChunkMap::iterator chunk_it = chunks_.begin(); chunk_it != chunks_.end(); ++chunk_it )
    {
        chunk_it->second->update_geometry();
    }

    SCOPE_TIMER_END
}

GameApplication::~GameApplication()
{
}

void GameApplication::handle_key_down_event( const int key, const int mod )
{
    bool handled = true;

    switch ( key )
    {
        case SDLK_LSHIFT:
            camera_.fast_move_mode( true );
            break;

        case SDLK_w:
            camera_.move_forward( true );
            break;

        case SDLK_s:
            camera_.move_backward( true );
            break;

        case SDLK_a:
            camera_.move_left( true );
            break;

        case SDLK_d:
            camera_.move_right( true );
            break;

        case SDLK_e:
            camera_.move_up( true );
            break;

        case SDLK_q:
            camera_.move_down( true );
            break;

        case SDLK_SPACE:
            printf( "Camera: pitch: %f, yaw: %f\n", camera_.pitch_, camera_.yaw_ );
            break;

        default:
            handled = false;
            break;
    }
}

void GameApplication::handle_key_up_event( const int key, const int mod )
{
    switch ( key )
    {
        case SDLK_LSHIFT:
            camera_.fast_move_mode( false );
            break;

        case SDLK_w:
            camera_.move_forward( false );
            break;

        case SDLK_s:
            camera_.move_backward( false );
            break;

        case SDLK_a:
            camera_.move_left( false );
            break;

        case SDLK_d:
            camera_.move_right( false );
            break;

        case SDLK_e:
            camera_.move_up( false );
            break;

        case SDLK_q:
            camera_.move_down( false );
            break;

        case SDLK_ESCAPE:
            run_ = false;
            break;

        case SDLK_F11:
            toggle_fullscreen();
            break;
    }
}

void GameApplication::handle_mouse_motion_event( const int button, const int x, const int y, const int xrel, const int yrel )
{
    // FIXME: SDL_WarpMouse generates a mouse motion event.  This is a somewhat ugly way to ignore it.
    static bool first_event = true;

    if ( first_event )
    {
        first_event = false;
        return;
    }

    camera_.handle_mouse_motion( xrel, yrel );
}

void GameApplication::handle_mouse_down_event( const int button, const int x, const int y, const int xrel, const int yrel )
{
    switch ( button )
    {
        case SDL_BUTTON_LEFT:
            break;
        case SDL_BUTTON_RIGHT:
            break;
    }
}

void GameApplication::do_one_step( float step_time )
{
    camera_.do_one_step( step_time );
}

void GameApplication::render()
{
    camera_.rotate();
    renderer_.render_skydome();
    camera_.translate();

    static bool first_time = true;

    if ( first_time )
    {
        SCOPE_TIMER_BEGIN( "First render" )
        renderer_.render_chunks( chunks_ );
        SCOPE_TIMER_END
        first_time = false;
    }
    else renderer_.render_chunks( chunks_ );
}
