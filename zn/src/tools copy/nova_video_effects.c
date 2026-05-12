/**
 * Nova Video Effects Library - Implementation
 */

#include "nova_video_effects.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

NovaFrame* nova_create_frame(int width, int height, int channels) {
    NovaFrame* frame = (NovaFrame*)malloc(sizeof(NovaFrame));
    if (!frame) return NULL;
    
    frame->width = width;
    frame->height = height;
    frame->channels = channels;
    frame->data = (unsigned char*)malloc(width * height * channels);
    
    if (!frame->data) {
        free(frame);
        return NULL;
    }
    
    return frame;
}

void nova_free_frame(NovaFrame* frame) {
    if (frame) {
        if (frame->data) free(frame->data);
        free(frame);
    }
}

void nova_copy_frame(const NovaFrame* src, NovaFrame* dst) {
    if (!src || !dst) return;
    memcpy(dst->data, src->data, src->width * src->height * src->channels);
}

// ============================================================================
// 3D PARALLAX EFFECT
// ============================================================================

int64_t nova_parallax_3d(
    const NovaFrame* frame,
    const unsigned char* depth_map,
    float offset_x,
    float offset_y,
    NovaFrame* output
) {
    if (!frame || !depth_map || !output) return -1;
    
    int w = frame->width;
    int h = frame->height;
    int ch = frame->channels;
    
    // Clear output
    memset(output->data, 0, w * h * ch);
    
    // Apply parallax based on depth
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            float depth = depth_map[idx] / 255.0f;  // 0=far, 1=near
            
            // Calculate parallax offset based on depth
            int src_x = x - (int)(offset_x * depth * 50);
            int src_y = y - (int)(offset_y * depth * 50);
            
            // Clamp to frame bounds
            if (src_x >= 0 && src_x < w && src_y >= 0 && src_y < h) {
                int src_idx = (src_y * w + src_x) * ch;
                int dst_idx = idx * ch;
                
                for (int c = 0; c < ch; c++) {
                    output->data[dst_idx + c] = frame->data[src_idx + c];
                }
            }
        }
    }
    
    return 0;
}

// ============================================================================
// KEN BURNS EFFECT
// ============================================================================

int64_t nova_ken_burns(
    const NovaFrame* frame,
    float zoom_factor,
    float pan_x,
    float pan_y,
    NovaFrame* output
) {
    if (!frame || !output) return -1;
    
    int w = frame->width;
    int h = frame->height;
    int ch = frame->channels;
    
    // Calculate zoomed dimensions
    int zoom_w = (int)(w / zoom_factor);
    int zoom_h = (int)(h / zoom_factor);
    
    // Calculate pan offset
    int offset_x = (int)((w - zoom_w) * (pan_x + 1.0f) / 2.0f);
    int offset_y = (int)((h - zoom_h) * (pan_y + 1.0f) / 2.0f);
    
    // Sample and scale
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // Map output pixel to input pixel
            int src_x = offset_x + (x * zoom_w / w);
            int src_y = offset_y + (y * zoom_h / h);
            
            // Clamp
            src_x = (src_x < 0) ? 0 : (src_x >= w) ? w-1 : src_x;
            src_y = (src_y < 0) ? 0 : (src_y >= h) ? h-1 : src_y;
            
            int src_idx = (src_y * w + src_x) * ch;
            int dst_idx = (y * w + x) * ch;
            
            for (int c = 0; c < ch; c++) {
                output->data[dst_idx + c] = frame->data[src_idx + c];
            }
        }
    }
    
    return 0;
}

int64_t nova_ken_burns_animate(
    const NovaFrame* frame,
    float start_zoom, float end_zoom,
    float start_x, float end_x,
    float start_y, float end_y,
    float progress,
    NovaFrame* output
) {
    // Interpolate parameters
    float zoom = start_zoom + (end_zoom - start_zoom) * progress;
    float pan_x = start_x + (end_x - start_x) * progress;
    float pan_y = start_y + (end_y - start_y) * progress;
    
    return nova_ken_burns(frame, zoom, pan_x, pan_y, output);
}

// ============================================================================
// MOTION BLUR
// ============================================================================

int64_t nova_motion_blur(
    const NovaFrame* frames,
    int num_frames,
    int direction,
    float intensity,
    NovaFrame* output
) {
    if (!frames || num_frames < 2 || !output) return -1;
    
    int w = frames[0].width;
    int h = frames[0].height;
    int ch = frames[0].channels;
    
    // Accumulate frames with weighted average
    memset(output->data, 0, w * h * ch);
    
    for (int f = 0; f < num_frames; f++) {
        float weight = 1.0f / num_frames;
        
        for (int i = 0; i < w * h * ch; i++) {
            output->data[i] += (unsigned char)(frames[f].data[i] * weight);
        }
    }
    
    return 0;
}

int64_t nova_directional_blur(
    const NovaFrame* frame,
    float velocity_x,
    float velocity_y,
    int samples,
    NovaFrame* output
) {
    if (!frame || !output || samples < 1) return -1;
    
    int w = frame->width;
    int h = frame->height;
    int ch = frame->channels;
    
    memset(output->data, 0, w * h * ch);
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int accum[4] = {0, 0, 0, 0};
            
            // Sample along motion vector
            for (int s = 0; s < samples; s++) {
                float t = (float)s / samples;
                int sx = x + (int)(velocity_x * t);
                int sy = y + (int)(velocity_y * t);
                
                if (sx >= 0 && sx < w && sy >= 0 && sy < h) {
                    int idx = (sy * w + sx) * ch;
                    for (int c = 0; c < ch; c++) {
                        accum[c] += frame->data[idx + c];
                    }
                }
            }
            
            int dst_idx = (y * w + x) * ch;
            for (int c = 0; c < ch; c++) {
                output->data[dst_idx + c] = accum[c] / samples;
            }
        }
    }
    
    return 0;
}

// ============================================================================
// SPEED RAMPING
// ============================================================================

int64_t nova_create_speed_curve(
    float start_speed,
    float end_speed,
    int duration,
    int curve_type,
    float* output
) {
    if (!output || duration < 1) return -1;
    
    for (int i = 0; i < duration; i++) {
        float t = (float)i / (duration - 1);
        float eased_t;
        
        switch (curve_type) {
            case 0: // Linear
                eased_t = t;
                break;
            case 1: // Ease-in (quadratic)
                eased_t = t * t;
                break;
            case 2: // Ease-out (quadratic)
                eased_t = t * (2.0f - t);
                break;
            case 3: // Ease-in-out (sine)
                eased_t = (1.0f - cosf(t * M_PI)) / 2.0f;
                break;
            default:
                eased_t = t;
        }
        
        output[i] = start_speed + (end_speed - start_speed) * eased_t;
    }
    
    return 0;
}

// ============================================================================
// CROSS DISSOLVE
// ============================================================================

int64_t nova_cross_dissolve(
    const NovaFrame* frame_a,
    const NovaFrame* frame_b,
    float mix,
    NovaFrame* output
) {
    if (!frame_a || !frame_b || !output) return -1;
    
    int size = frame_a->width * frame_a->height * frame_a->channels;
    
    for (int i = 0; i < size; i++) {
        output->data[i] = (unsigned char)(
            frame_a->data[i] * (1.0f - mix) + 
            frame_b->data[i] * mix
        );
    }
    
    return 0;
}

// ============================================================================
// COLOR GRADING
// ============================================================================

int64_t nova_apply_lut(
    const NovaFrame* frame,
    const unsigned char* lut_data,
    int lut_size,
    float intensity,
    NovaFrame* output
) {
    if (!frame || !lut_data || !output) return -1;
    
    int w = frame->width;
    int h = frame->height;
    
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 3;
            
            // Get RGB values
            int r = frame->data[idx + 0];
            int g = frame->data[idx + 1];
            int b = frame->data[idx + 2];
            
            // Map to LUT coordinates
            int lr = (r * (lut_size - 1)) / 255;
            int lg = (g * (lut_size - 1)) / 255;
            int lb = (b * (lut_size - 1)) / 255;
            
            // Lookup in 3D LUT
            int lut_idx = (lb * lut_size * lut_size + lg * lut_size + lr) * 3;
            
            // Apply with intensity
            output->data[idx + 0] = r * (1.0f - intensity) + lut_data[lut_idx + 0] * intensity;
            output->data[idx + 1] = g * (1.0f - intensity) + lut_data[lut_idx + 1] * intensity;
            output->data[idx + 2] = b * (1.0f - intensity) + lut_data[lut_idx + 2] * intensity;
        }
    }
    
    return 0;
}

int64_t nova_create_cinematic_lut(
    int preset,
    unsigned char* lut_data,
    int lut_size
) {
    if (!lut_data) return -1;
    
    // Generate 3D LUT based on preset
    for (int b = 0; b < lut_size; b++) {
        for (int g = 0; g < lut_size; g++) {
            for (int r = 0; r < lut_size; r++) {
                int idx = (b * lut_size * lut_size + g * lut_size + r) * 3;
                
                // Scale to 0-255
                float rf = (float)r / (lut_size - 1);
                float gf = (float)g / (lut_size - 1);
                float bf = (float)b / (lut_size - 1);
                
                // Apply preset transform
                switch (preset) {
                    case 0: // Warm
                        rf *= 1.1f;
                        bf *= 0.9f;
                        break;
                    case 1: // Cool
                        rf *= 0.9f;
                        bf *= 1.1f;
                        break;
                    case 2: // Vintage
                        rf = powf(rf, 1.2f);
                        gf = powf(gf, 1.1f);
                        bf = powf(bf, 0.9f);
                        break;
                    case 3: // Modern (high contrast)
                        rf = powf(rf, 1.3f);
                        gf = powf(gf, 1.3f);
                        bf = powf(bf, 1.3f);
                        break;
                    case 4: // Teal-Orange
                        if (rf > 0.5f) rf *= 1.2f;
                        if (bf > 0.5f) bf *= 1.2f;
                        break;
                }
                
                // Clamp and store
                lut_data[idx + 0] = (unsigned char)(fminf(rf * 255, 255));
                lut_data[idx + 1] = (unsigned char)(fminf(gf * 255, 255));
                lut_data[idx + 2] = (unsigned char)(fminf(bf * 255, 255));
            }
        }
    }
    
    return 0;
}

int64_t nova_color_correct(
    const NovaFrame* frame,
    float exposure,
    float contrast,
    float saturation,
    float temperature,
    NovaFrame* output
) {
    if (!frame || !output) return -1;
    
    int size = frame->width * frame->height;
    
    for (int i = 0; i < size; i++) {
        int idx = i * 3;
        
        float r = frame->data[idx + 0] / 255.0f;
        float g = frame->data[idx + 1] / 255.0f;
        float b = frame->data[idx + 2] / 255.0f;
        
        // Exposure
        float exp_mult = powf(2.0f, exposure);
        r *= exp_mult;
        g *= exp_mult;
        b *= exp_mult;
        
        // Contrast
        r = (r - 0.5f) * contrast + 0.5f;
        g = (g - 0.5f) * contrast + 0.5f;
        b = (b - 0.5f) * contrast + 0.5f;
        
        // Saturation
        float lum = 0.299f * r + 0.587f * g + 0.114f * b;
        r = lum + (r - lum) * saturation;
        g = lum + (g - lum) * saturation;
        b = lum + (b - lum) * saturation;
        
        // Temperature
        r += temperature * 0.01f;
        b -= temperature * 0.01f;
        
        // Clamp
        r = fmaxf(0.0f, fminf(1.0f, r));
        g = fmaxf(0.0f, fminf(1.0f, g));
        b = fmaxf(0.0f, fminf(1.0f, b));
        
        output->data[idx + 0] = (unsigned char)(r * 255);
        output->data[idx + 1] = (unsigned char)(g * 255);
        output->data[idx + 2] = (unsigned char)(b * 255);
    }
    
    return 0;
}
