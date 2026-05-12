/*
 * Image Loader Implementation
 * Uses stb_image for JPEG/PNG loading
 */

#include "image_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// ============================================================================
// IMAGE OPERATIONS (stb_image)
// ============================================================================

Image *load_image(const char *filepath) {
  int w, h, c;
  uint8_t *data = stbi_load(filepath, &w, &h, &c, 3); // Force 3 channels (RGB)
  if (!data) {
    fprintf(stderr, "Failed to load image: %s (%s)\n", filepath,
            stbi_failure_reason());
    return NULL;
  }

  Image *img = (Image *)malloc(sizeof(Image));
  img->width = w;
  img->height = h;
  img->channels = 3;
  img->data = data; // stb_image handles allocation

  printf("   📷 Loaded: %s (%dx%d RGB)\n", filepath, w, h);
  return img;
}

char *load_caption(const char *image_path) {
  char txt_path[1024];
  strncpy(txt_path, image_path, sizeof(txt_path));
  char *dot = strrchr(txt_path, '.');
  if (dot) {
    strcpy(dot, ".txt");
  } else {
    strcat(txt_path, ".txt");
  }

  FILE *f = fopen(txt_path, "r");
  if (!f) {
    return strdup("a professional photo"); // Default caption
  }

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  rewind(f);

  char *caption = (char *)malloc(size + 1);
  fread(caption, 1, size, f);
  caption[size] = '\0';
  fclose(f);

  // Clean newline
  char *nl = strchr(caption, '\n');
  if (nl)
    *nl = '\0';

  return caption;
}

void free_image(Image *img) {
  if (img) {
    stbi_image_free(img->data);
    free(img);
  }
}

Image *resize_image(Image *img, int target_width, int target_height) {
  // Simple nearest-neighbor resize
  Image *resized = (Image *)malloc(sizeof(Image));
  resized->width = target_width;
  resized->height = target_height;
  resized->channels = img->channels;

  size_t size = (size_t)target_width * target_height * resized->channels;
  resized->data = (uint8_t *)malloc(size);

  if (!resized->data) {
    fprintf(stderr, "ERROR: Failed to allocate %zu bytes for resize\n", size);
    free(resized);
    return NULL;
  }

  float x_ratio = (float)img->width / target_width;
  float y_ratio = (float)img->height / target_height;

  for (int y = 0; y < target_height; y++) {
    for (int x = 0; x < target_width; x++) {
      int src_x = (int)(x * x_ratio);
      int src_y = (int)(y * y_ratio);

      for (int c = 0; c < resized->channels; c++) {
        int dst_idx = (y * target_width + x) * resized->channels + c;
        int src_idx = (src_y * img->width + src_x) * img->channels + c;
        resized->data[dst_idx] = img->data[src_idx];
      }
    }
  }

  return resized;
}

// ============================================================================
// NOVA TENSOR CONVERSION
// ============================================================================

NovaTensor *image_to_tensor(NovaContext *ctx, Image *img) {
  // Convert [H, W, C] uint8 to [C, H, W] float normalized
  int64_t shape[3] = {img->channels, img->height, img->width};
  NovaTensor *tensor = nova_tensor_create(ctx, shape, 3, NOVA_DTYPE_FP32);
  float *t_data = (float *)tensor->data;

  // Normalize from [0, 255] to [-1, 1] (ImageNet style)
  for (int c = 0; c < img->channels; c++) {
    for (int h = 0; h < img->height; h++) {
      for (int w = 0; w < img->width; w++) {
        int img_idx = (h * img->width + w) * img->channels + c;
        int tensor_idx = c * (img->height * img->width) + h * img->width + w;

        // Normalize: (pixel / 255.0) * 2.0 - 1.0
        float normalized = ((float)img->data[img_idx] / 255.0f) * 2.0f - 1.0f;
        t_data[tensor_idx] = normalized;
      }
    }
  }

  return tensor;
}

NovaTensor *load_image_as_tensor(NovaContext *ctx, const char *filepath,
                                   int size) {
  Image *img = load_image(filepath);
  if (!img)
    return NULL;

  // Resize if needed
  Image *resized = NULL;
  if (img->width != size || img->height != size) {
    resized = resize_image(img, size, size);
    free_image(img);
    img = resized;
  }

  NovaTensor *tensor = image_to_tensor(ctx, img);
  free_image(img);

  return tensor;
}

// ============================================================================
// BATCH LOADING
// ============================================================================

NovaTensor *load_batch_images(NovaContext *ctx, const char **filepaths,
                                int batch_size, int img_size) {
  printf("   📦 Loading batch of %d images (%dx%d)...\n", batch_size, img_size,
         img_size);

  // Output: [batch_size, 3, img_size, img_size]
  int64_t shape[4] = {batch_size, 3, img_size, img_size};
  NovaTensor *batch = nova_tensor_create(ctx, shape, 4, NOVA_DTYPE_FP32);
  float *b_data = (float *)batch->data;

  int img_elements = 3 * img_size * img_size;

  for (int i = 0; i < batch_size; i++) {
    NovaTensor *img_tensor =
        load_image_as_tensor(ctx, filepaths[i], img_size);
    if (!img_tensor)
      continue;

    // Copy to batch
    memcpy(b_data + i * img_elements, img_tensor->data,
           img_elements * sizeof(float));

    nova_tensor_destroy(img_tensor);
  }

  printf("      ✅ Batch loaded: [%d, 3, %d, %d]\n", batch_size, img_size,
         img_size);

  return batch;
}

// ============================================================================
// DATASET HELPERS
// ============================================================================

// Helper for recursion
static void scan_dir_recursive(const char *dir, char ***files, int *count,
                               int *cap) {
  DIR *d = opendir(dir);
  if (!d)
    return;

  struct dirent *entry;
  while ((entry = readdir(d)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);

    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
      scan_dir_recursive(path, files, count, cap);
    } else {
      const char *name = entry->d_name;
      int len = strlen(name);
      if (len > 4) {
        if (strcasecmp(name + len - 4, ".jpg") == 0 ||
            strcasecmp(name + len - 4, ".png") == 0 ||
            strcasecmp(name + len - 4, ".jpeg") == 0) {

          if (*count >= *cap) {
            *cap = (*cap == 0) ? 10 : (*cap * 2);
            *files = (char **)realloc(*files, *cap * sizeof(char *));
          }
          (*files)[(*count)++] = strdup(path);
        }
      }
    }
  }
  closedir(d);
}

int count_images_in_directory(const char *dir) {
  int count = 0, cap = 0;
  char **files = NULL;
  scan_dir_recursive(dir, &files, &count, &cap);
  for (int i = 0; i < count; i++)
    free(files[i]);
  free(files);
  return count;
}

char **list_images_in_directory(const char *dir, int *count) {
  int cap = 10;
  char **files = (char **)malloc(cap * sizeof(char *));
  *count = 0;
  scan_dir_recursive(dir, &files, count, &cap);
  if (*count == 0) {
    free(files);
    return NULL;
  }
  return files;
}
