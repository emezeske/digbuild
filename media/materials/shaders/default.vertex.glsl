uniform vec3 sun_direction;
uniform vec3 moon_direction;
uniform vec3 sun_light_color;
uniform vec3 moon_light_color;

varying vec3 lighting;
varying vec2 texture_coordinates;

void main()
{
    const vec3 ambient_light = vec3( 0.1, 0.1, 0.1 );
    vec4 light_level = gl_MultiTexCoord1;
    float sun_incidence = 0.50 + 0.50 * dot( sun_direction, gl_Normal );
    float moon_incidence = 0.50 + 0.50 * dot( moon_direction, gl_Normal );
    vec3 sun_lighting = sun_light_color * light_level.a * sun_incidence;
    vec3 moon_lighting = moon_light_color * light_level.a * moon_incidence;

    lighting = ambient_light + light_level.rgb + sun_lighting + moon_lighting;
    texture_coordinates = gl_MultiTexCoord0.st;

    gl_Position = ftransform();
}
