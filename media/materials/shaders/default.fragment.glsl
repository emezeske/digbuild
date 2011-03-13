uniform sampler2D material_texture;
uniform sampler2D material_specular_map;
uniform sampler2D material_bump_map;

varying vec3 tangent_sun_direction;
varying vec3 tangent_camera_direction;
varying vec3 base_lighting;
varying vec3 sun_lighting;
varying vec2 texture_coordinates;

void main()
{
    vec4 texture_color = texture2D( material_texture, texture_coordinates );

    // The bump map uses the 'z' coordinate to represent height, while Digbuild uses the 'y'
    // coordinate; thus the bump map needs to be swizzled a bit.
    vec3 bump_direction = texture2D( material_bump_map, texture_coordinates ).xzy;
    // The bump map 'x' and 'z' coordinates are packed into the range [0,1] so they need to be
    // massaged into their real range of [-1,1].  The 'y' coordinate is not packed this way.
    bump_direction.x = bump_direction.x * 2.0f - 1.0f;
    bump_direction.z = bump_direction.z * 2.0f - 1.0f;
    bump_direction = normalize( bump_direction );

    float sun_bump = 0.25f + 0.75f * clamp( dot( tangent_sun_direction, bump_direction ), 0.0f, 1.0f );

    vec3 reflected_sun_direction = reflect( -tangent_sun_direction, bump_direction );
    float sun_specularity = max( dot( tangent_camera_direction, reflected_sun_direction ), 0.0f );
    float material_specularity = texture2D( material_specular_map, texture_coordinates ).r;
    vec3 sun_specular = material_specularity * pow( sun_specularity, 16 ) * sun_lighting;

    // FIXME: The bump factor should only affect light from the sun.
    gl_FragColor = vec4( texture_color.rgb * base_lighting * sun_bump + sun_specular, texture_color.a );
}
