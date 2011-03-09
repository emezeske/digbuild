uniform vec3 atmospheric_light_direction;
uniform vec3 atmospheric_light_color;

varying vec3 lighting;
varying vec2 texture_coordinates;

void main()
{
    const vec3 ambient_light = vec3( 0.1, 0.1, 0.1 );
    vec4 light_level = gl_MultiTexCoord1;
    float atmospheric_light_incidence = 0.50 + 0.50 * dot( atmospheric_light_direction, gl_Normal );

    lighting = ambient_light + light_level.rgb + atmospheric_light_color * light_level.a * atmospheric_light_incidence;
    texture_coordinates = gl_MultiTexCoord0.st;

    gl_Position = ftransform();
}
