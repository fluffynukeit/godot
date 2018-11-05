
[vertex]

// vertex is in world cordinates
layout(location = 0) in vec3 vertex_attrib;

layout(std140) uniform SceneData { // ubo:0

        highp mat4 projection_matrix;
        highp mat4 inv_projection_matrix;
        highp mat4 camera_inverse_matrix;
        highp mat4 camera_matrix;

        mediump vec4 ambient_light_color;
        mediump vec4 bg_color;

        mediump vec4 fog_color_enabled;
        mediump vec4 fog_sun_color_amount;

        mediump float ambient_energy;
        mediump float bg_energy;

        mediump float z_offset;
        mediump float z_slope_scale;
        highp float shadow_dual_paraboloid_render_zfar;
        highp float shadow_dual_paraboloid_render_side;

        highp vec2 viewport_size;
        highp vec2 screen_pixel_size;
        highp vec2 shadow_atlas_pixel_size;
        highp vec2 directional_shadow_pixel_size;

        highp float time;
        highp float z_far;
        mediump float reflection_multiplier;
        mediump float subsurface_scatter_width;
        mediump float ambient_occlusion_affect_light;
        mediump float ambient_occlusion_affect_ao_channel;
        mediump float opaque_prepass_threshold;

        bool fog_depth_enabled;
        highp float fog_depth_begin;
        highp float fog_depth_curve;
        bool fog_transmit_enabled;
        highp float fog_transmit_curve;
        bool fog_height_enabled;
        highp float fog_height_min;
        highp float fog_height_max;
        highp float fog_height_curve;
};

uniform float point_scale;
uniform float particle_radius;
uniform vec3 color;

out highp vec4 out_color; //tfb:

void main(){

    vec4 screen_space_pos =
            projection_matrix *
            camera_inverse_matrix *
            vec4(vertex_attrib, 1.0);

    // Print on top
    // screen_space_pos.z = 0.99;

    gl_Position = screen_space_pos;

    gl_PointSize = point_scale * (particle_radius / gl_Position.w);


    out_color = vec4(color, 1);
}

[fragment]

in vec4 color;

out vec4 FragColor;

void main() {

    FragColor = color;
}
