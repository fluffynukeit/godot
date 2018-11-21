
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

out highp vec3 cam_space_point_position;

void main(){

    highp vec4 cam_space_pos =
            camera_inverse_matrix *
            vec4(vertex_attrib, 1.0);

    highp vec4 clip_space_pos =
            projection_matrix *
            cam_space_pos;

    gl_Position = clip_space_pos;

    gl_PointSize = point_scale * (particle_radius / clip_space_pos.w);

    cam_space_point_position = cam_space_pos.xyz;
}

[fragment]

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

in highp vec3 cam_space_point_position;

layout(location = 0) out vec4 normal_depth;

void main() {

    // Normal
    vec3 normal;
    normal.xy = gl_PointCoord * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    float mag = dot(normal.xy, normal.xy);
    if (mag > 1.0)
        discard;
    normal.z = sqrt(1.0 - mag);

    // Depth
    vec3 cs_frag_pos = cam_space_point_position + normal.xyz * particle_radius;
    vec4 screen_frag_pos = projection_matrix * vec4(cs_frag_pos, 1.0);

    normal_depth.xyz = normal;
    normal_depth.w = screen_frag_pos.z / z_far; // Depth

    gl_FragDepth = normal_depth.w;

}
