uniform sampler2D material_texture;
uniform sampler2D material_specular_map;
uniform sampler2D material_bump_map;

varying vec3 tangent_sun_direction;
varying vec3 base_lighting;
varying vec3 sun_lighting;
varying float sun_specular_intensity;
varying vec2 texture_coordinates;

void main()
{
    vec4 texture_color = texture2D( material_texture, texture_coordinates );
    vec3 bump_direction = normalize( texture2D( material_bump_map, texture_coordinates ).rgb * 2.0f - 1.0f );
    float specular_intensity = texture2D( material_specular_map, texture_coordinates ).r;
    vec3 sun_specular = specular_intensity * pow( sun_specular_intensity, 15.0f ) * sun_lighting;
    float sun_bump = 0.5f + 0.5f * clamp( dot( tangent_sun_direction, bump_direction ), 0.0f, 1.0f );

    gl_FragColor = vec4( texture_color.rgb * base_lighting * sun_bump + sun_specular, texture_color.a );
}
