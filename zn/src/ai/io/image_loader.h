#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include "nova_tensor.h"
#include <stdint.h>

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * Image Loader for Nova AI Training
 * Loads RGB images and converts to NovaTensors
 * ═══════════════════════════════════════════════════════════════════════════
 */

// Image structure
typedef struct {
  uint8_t *data; // RGB data
  int width;
  int height;
  int channels; // 3 for RGB
} Image;

// Image loading
Image *load_image(const char *filepath);
char *load_caption(const char *image_path);
void free_image(Image *img);

// Image processing
Image *resize_image(Image *img, int target_width, int target_height);
Image *normalize_image(Image *img); // Scale to [0, 1]

// Convert to NovaTensor
NovaTensor *image_to_tensor(NovaContext *ctx,
                              Image *img); // [H, W, C] -> [C, H, W] normalized
NovaTensor *load_image_as_tensor(NovaContext *ctx, const char *filepath,
                                   int size); // Direct load

// Batch loading for training
NovaTensor *load_batch_images(NovaContext *ctx, const char **filepaths,
                                int batch_size, int img_size);

// Dataset helpers
int count_images_in_directory(const char *dir);
char **list_images_in_directory(const char *dir, int *count);

#endif // IMAGE_LOADER_H
