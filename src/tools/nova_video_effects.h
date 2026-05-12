/**
 * Nova Video Effects Library
 * Advanced video processing effects for cinematic output
 */

#ifndef NOVA_VIDEO_EFFECTS_H
#define NOVA_VIDEO_EFFECTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Frame buffer structure
typedef struct {
    unsigned char* data;  // RGB or RGBA data
    int width;
    int height;
    int channels;  // 3 for RGB, 4 for RGBA
} NovaFrame;

// ============================================================================
// 3D PARALLAX EFFECT
// ============================================================================

/**
 * Apply 3D parallax depth effect
 * Creates illusion of depth by moving layers at different speeds
 * 
 * @param frame Input frame
 * @param depth_map Grayscale depth map (0=far, 255=near)
 * @param offset_x Horizontal camera offset
 * @param offset_y Vertical camera offset
 * @param output Output frame
 * @return 0 on success
 */
int64_t nova_parallax_3d(
    const NovaFrame* frame,
    const unsigned char* depth_map,
    float offset_x,
    float offset_y,
    NovaFrame* output
);

// ============================================================================
// KEN BURNS EFFECT (Pan + Zoom)
// ============================================================================

/**
 * Apply Ken Burns pan and zoom effect
 * 
 * @param frame Input frame
 * @param zoom_factor Zoom level (1.0 = no zoom, >1.0 = zoom in)
 * @param pan_x Horizontal pan offset (-1.0 to 1.0)
 * @param pan_y Vertical pan offset (-1.0 to 1.0)
 * @param output Output frame
 * @return 0 on success
 */
int64_t nova_ken_burns(
    const NovaFrame* frame,
    float zoom_factor,
    float pan_x,
    float pan_y,
    NovaFrame* output
);

/**
 * Animate Ken Burns effect between keyframes
 * 
 * @param frame Input frame
 * @param start_zoom Initial zoom level
 * @param end_zoom Final zoom level
 * @param start_x Initial pan X
 * @param end_x Final pan X
 * @param start_y Initial pan Y
 * @param end_y Final pan Y
 * @param progress Animation progress (0.0 to 1.0)
 * @param output Output frame
 * @return 0 on success
 */
int64_t nova_ken_burns_animate(
    const NovaFrame* frame,
    float start_zoom, float end_zoom,
    float start_x, float end_x,
    float start_y, float end_y,
    float progress,
    NovaFrame* output
);

// ============================================================================
// MOTION BLUR
// ============================================================================

/**
 * Apply motion blur effect
 * Simulates camera motion blur
 * 
 * @param frames Array of consecutive frames
 * @param num_frames Number of frames to blend
 * @param direction Blur direction (0=horizontal, 1=vertical, 2=radial)
 * @param intensity Blur intensity (0.0 to 1.0)
 * @param output Output frame
 * @return 0 on success
 */
int64_t nova_motion_blur(
    const NovaFrame* frames,
    int num_frames,
    int direction,
    float intensity,
    NovaFrame* output
);

/**
 * Apply directional motion blur
 * 
 * @param frame Input frame
 * @param velocity_x Horizontal motion velocity
 * @param velocity_y Vertical motion velocity
 * @param samples Number of blur samples
 * @param output Output frame
 * @return 0 on success
 */
int64_t nova_directional_blur(
    const NovaFrame* frame,
    float velocity_x,
    float velocity_y,
    int samples,
    NovaFrame* output
);

// ============================================================================
// SPEED RAMPING
// ============================================================================

/**
 * Apply speed ramping (time remapping)
 * 
 * @param frames Array of input frames
 * @param num_frames Number of frames
 * @param time_curve Speed curve (array of time values)
 * @param curve_length Length of time curve
 * @param output_frames Output frame buffer
 * @param num_output Number of output frames
 * @return Number of frames generated
 */
int64_t nova_speed_ramp(
    const NovaFrame* frames,
    int num_frames,
    const float* time_curve,
    int curve_length,
    NovaFrame* output_frames,
    int num_output
);

/**
 * Create smooth speed ramp curve
 * 
 * @param start_speed Initial playback speed
 * @param end_speed Final playback speed
 * @param duration Ramp duration in frames
 * @param curve_type 0=linear, 1=ease-in, 2=ease-out, 3=ease-in-out
 * @param output Output curve buffer
 * @return 0 on success
 */
int64_t nova_create_speed_curve(
    float start_speed,
    float end_speed,
    int duration,
    int curve_type,
    float* output
);

// ============================================================================
// MATCH CUT + CROSS DISSOLVE
// ============================================================================

/**
 * Cross dissolve transition between frames
 * 
 * @param frame_a First frame
 * @param frame_b Second frame
 * @param mix Blend amount (0.0 = frame_a, 1.0 = frame_b)
 * @param output Output frame
 * @return 0 on success
 */
int64_t nova_cross_dissolve(
    const NovaFrame* frame_a,
    const NovaFrame* frame_b,
    float mix,
    NovaFrame* output
);

/**
 * Match cut with shape/content matching
 * Intelligently transitions based on visual similarity
 * 
 * @param frame_a First frame
 * @param frame_b Second frame
 * @param match_regions Array of matching region pairs
 * @param num_regions Number of matching regions
 * @param progress Transition progress (0.0 to 1.0)
 * @param output Output frame
 * @return 0 on success
 */
int64_t nova_match_cut(
    const NovaFrame* frame_a,
    const NovaFrame* frame_b,
    const int* match_regions,
    int num_regions,
    float progress,
    NovaFrame* output
);

// ============================================================================
// COLOR GRADING (LUT)
// ============================================================================

/**
 * Apply color grading using LUT (Look-Up Table)
 * 
 * @param frame Input frame
 * @param lut_data 3D LUT data (64x64x64 RGB cube typical)
 * @param lut_size Size of LUT cube (e.g., 64)
 * @param intensity Effect intensity (0.0 to 1.0)
 * @param output Output frame
 * @return 0 on success
 */
int64_t nova_apply_lut(
    const NovaFrame* frame,
    const unsigned char* lut_data,
    int lut_size,
    float intensity,
    NovaFrame* output
);

/**
 * Create cinematic color grade LUT
 * 
 * @param preset Preset type (0=warm, 1=cool, 2=vintage, 3=modern, 4=teal-orange)
 * @param lut_data Output LUT buffer
 * @param lut_size Size of LUT cube
 * @return 0 on success
 */
int64_t nova_create_cinematic_lut(
    int preset,
    unsigned char* lut_data,
    int lut_size
);

/**
 * Apply basic color corrections
 * 
 * @param frame Input frame
 * @param exposure Exposure adjustment (-2.0 to 2.0)
 * @param contrast Contrast adjustment (0.0 to 2.0)
 * @param saturation Saturation adjustment (0.0 to 2.0)
 * @param temperature Color temperature (-100 to 100)
 * @param output Output frame
 * @return 0 on success
 */
int64_t nova_color_correct(
    const NovaFrame* frame,
    float exposure,
    float contrast,
    float saturation,
    float temperature,
    NovaFrame* output
);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Create frame buffer
 */
NovaFrame* nova_create_frame(int width, int height, int channels);

/**
 * Free frame buffer
 */
void nova_free_frame(NovaFrame* frame);

/**
 * Copy frame
 */
void nova_copy_frame(const NovaFrame* src, NovaFrame* dst);

#ifdef __cplusplus
}
#endif

#endif // NOVA_VIDEO_EFFECTS_H
