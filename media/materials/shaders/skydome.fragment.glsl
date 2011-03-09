uniform vec3 zenith_color;
uniform vec3 horizon_color;
uniform float skydome_radius;

varying float height;

void main()
{
    float height_factor = pow( 1.0f - ( height ) / skydome_radius, 1.75f );
    vec3 sky_color = mix( zenith_color, horizon_color, clamp( height_factor, 0.0f, 1.0f ) );
    gl_FragColor = vec4( sky_color, 0.0f );
}
