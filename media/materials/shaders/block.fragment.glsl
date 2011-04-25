uniform sampler2DArray material_texture_array;
uniform sampler2DArray material_specular_map_array;
uniform sampler2DArray material_bump_map_array;
uniform float fog_distance;

varying vec3 tangent_sun_direction;
varying vec3 tangent_camera_direction;
varying vec3 base_lighting;
varying vec3 sun_lighting;
varying vec3 texture_coordinates;
varying float fog_depth;

void main()
{
    vec4 texture_color = texture2DArray( material_texture_array, texture_coordinates );

    // The bump map uses the 'z' coordinate to represent height, while Digbuild uses the 'y'
    // coordinate; thus the bump map needs to be swizzled a bit.
    vec3 bump_direction = texture2DArray( material_bump_map_array, texture_coordinates ).xzy;
    // The bump map 'x' and 'z' coordinates are packed into the range [0,1] so they need to be
    // massaged into their real range of [-1,1].  The 'y' coordinate is not packed this way.
    bump_direction.x = bump_direction.x * 2.0f - 1.0f;
    bump_direction.z = bump_direction.z * 2.0f - 1.0f;
    bump_direction = normalize( bump_direction );

    float bump_factor = clamp( dot( tangent_sun_direction, bump_direction ), 0.0f, 1.0f );
    vec3 sun_bump = 0.50f * bump_factor * sun_lighting;

    vec3 reflected_sun_direction = reflect( -tangent_sun_direction, bump_direction );
    float sun_specularity = max( dot( tangent_camera_direction, reflected_sun_direction ), 0.0f );
    float material_specularity = texture2DArray( material_specular_map_array, texture_coordinates ).r;
    vec3 sun_specular = material_specularity * pow( sun_specularity, 16 ) * sun_lighting;

    vec3 color = texture_color.rgb * ( base_lighting + sun_bump ) + sun_specular;
    // FIXME: Making the distant geometry fade out via alpha looks nice, but it will
    //        do really weird stuff if, e.g., a player is inside a really tall house --
    //        the sky will show through the roof!
    float fog_factor = clamp( ( fog_distance - fog_depth ) * 0.20f, 0.0f, 1.0f );
    gl_FragColor = vec4( color, fog_factor * texture_color.a );
}
