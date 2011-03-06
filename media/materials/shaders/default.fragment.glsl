uniform sampler2D texture;

void main()
{
    const vec4 ambient = vec4( 0.1, 0.1, 0.1, 1.0 );
    vec4 texture_color = texture2D( texture, gl_TexCoord[0].st );
    vec4 lighting = ambient + vec4( gl_TexCoord[1].stp, 1.0 );
    gl_FragColor = texture_color * lighting;
}
