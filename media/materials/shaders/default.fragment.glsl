uniform sampler2D texture;

varying vec3 lighting;
varying vec2 texture_coordinates;

void main()
{
    vec4 texture_color = texture2D( texture, texture_coordinates );
    gl_FragColor = vec4( texture_color.rgb * lighting, texture_color.a );
}
