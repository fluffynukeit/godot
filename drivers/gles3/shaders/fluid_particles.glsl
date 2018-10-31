
[vertex]

out highp vec4 out_color; //tfb:

uniform vec3 color;

void main(){

    out_color = vec4(color, 1);
    gl_Position = vec4(0.0, 0.0, 0.9, 1.0);
    gl_PointSize = 20.0;
}

[fragment]

in vec4 color;

out vec4 FragColor;

void main() {

    //FragColor = color;
    FragColor = vec4(0,1,0,1);
}
