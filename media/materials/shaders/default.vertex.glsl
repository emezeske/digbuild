uniform vec3 camera_position;
uniform vec3 sun_direction;
uniform vec3 moon_direction;
uniform vec3 sun_light_color;
uniform vec3 moon_light_color;

varying vec3 tangent_sun_direction;
varying vec3 tangent_camera_direction;
varying vec3 sun_lighting;
varying vec3 base_lighting;
varying vec2 texture_coordinates;

void main()
{
    // TODO: Use a *much* less expensive shader for more distant geometry.  The bump/specular mapping
    //       performed by this shader is pointless once the viewer is a few dozen meters away.

    // A TBN matrix can be used to transform coordinates from tangent space to object space.  However,
    // we want to do the exact opposite of that -- we want to go from object space to tangent space.
    // So we need inverse(TBN).  Luckily, the block faces all have orthonormal TBN matrices, so
    // inverse(TBN) == transpose(TBN), which can be calulated much more efficiently.
    //
    // NOTE: Digbuild actually uses a TNB matrix, because it uses the 'y' coordinate to represent height.

    vec3 tangent = gl_MultiTexCoord0.xyz;
    vec3 bitangent = cross( gl_Normal, tangent );
    mat3 tbn_transpose = transpose( mat3( tangent, gl_Normal, bitangent ) );
    tangent_sun_direction = normalize( tbn_transpose * sun_direction );
    tangent_camera_direction = normalize( tbn_transpose * ( camera_position - gl_Vertex.xyz ) );

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
