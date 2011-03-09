varying float height;

void main()
{
    height = gl_Vertex.y;
    gl_Position = ftransform();
}
