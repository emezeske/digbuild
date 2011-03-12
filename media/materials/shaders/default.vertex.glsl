uniform vec3 sun_direction;
uniform vec3 moon_direction;
uniform vec3 sun_light_color;
uniform vec3 moon_light_color;

varying vec3 tangent_sun_direction;
varying float sun_specular_intensity;
varying vec3 sun_lighting;
varying vec3 base_lighting;
varying vec2 texture_coordinates;

void main()
{
    // TODO: Use a *much* less expensive shader for more distant geometry.  The bump/specular mapping
    //       performed by this shader is pointless once the viewer is a few dozen meters away.

    // TODO: Maybe use the bnt matrix to move the camera/sun directions into tangent space,
    //       and then in the fragment shader the specular intensity can be dependent upon
    //       the bump direction.
    vec3 eyespace_camera_direction = normalize( vec3( gl_ModelViewMatrix * gl_Vertex ) );
    vec3 eyespace_sun_direction = normalize( vec3( gl_ModelViewMatrix * vec4( reflect( sun_direction, gl_Normal ), 0.0f ) ) );
    sun_specular_intensity = max( dot( eyespace_sun_direction, eyespace_camera_direction ), 0.0f );

    // Create a BNT matrix to transform the Sun's direction into the vertex' tangent space.
    vec3 tangent = gl_MultiTexCoord0.xyz;
    vec3 bitangent = cross( gl_Normal, tangent );
    mat3 bnt = mat3( bitangent, gl_Normal, tangent );
    tangent_sun_direction = bnt * sun_direction;

    vec4 light_level = gl_MultiTexCoord2;
    sun_lighting = sun_light_color * light_level.a;

    float sun_incidence = 0.65f + 0.35f * dot( sun_direction, gl_Normal );
    vec3 sun_diffuse = sun_lighting * sun_incidence;

    float moon_incidence = 0.65f + 0.35f * dot( moon_direction, gl_Normal );
    vec3 moon_diffuse = moon_light_color * light_level.a * moon_incidence;

    const vec3 ambient_light = vec3( 0.03f, 0.03f, 0.03f );
    base_lighting = ambient_light + light_level.rgb + sun_diffuse + moon_diffuse;

    texture_coordinates = gl_MultiTexCoord1.st;

    gl_Position = ftransform();
}
