uniform vec3 sun_direction;
uniform vec3 sun_color;

varying vec3 lighting;
varying vec2 texture_coordinates;

void main()
{
    const vec3 ambient_light = vec3( 0.1, 0.1, 0.1 );
    vec4 light_level = gl_MultiTexCoord1;
    float sunlight_incidence = 0.75 + 0.25 * dot( sun_direction, gl_Normal );

    lighting = ambient_light + light_level.rgb + sun_color * light_level.a * sunlight_incidence;
    texture_coordinates = gl_MultiTexCoord0.st;

    gl_Position = ftransform();
}
