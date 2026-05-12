#include "image_loader.h"
#include "safetensors_loader.h"
#include "runtime/nova_context.h"
#include "nova_tensor_ops.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// ============================================
// NOVA FFI TYPES & HELPERS
// ============================================

// Nova strings are (ptr, len)
typedef struct {
  const char *ptr;
  int64_t len;
} ZString;

static char *zstr_to_c(ZString s) { return strndup(s.ptr, s.len); }

// ============================================
// IO FUNCTIONS
// ============================================

NovaTensor *io_read_image(NovaContext *ctx, ZString path) {
  char *cpath = zstr_to_c(path);
  NovaTensor *t = load_image_as_tensor(ctx, cpath, 512);
  free(cpath);
  return t;
}

void io_write_png(ZString path, NovaTensor *img) {
  // We'll use PPM for now as PNG needs a library
  char *cpath = zstr_to_c(path);
  char ppm_path[256];
  snprintf(ppm_path, sizeof(ppm_path), "%s.ppm", cpath);

  FILE *f = fopen(ppm_path, "wb");
  if (!f)
    return;
  fprintf(f, "P6\n%lld %lld\n255\n", img->shape[2], img->shape[1]);
  float *data = (float *)img->data;
  for (int64_t i = 0; i < img->total_elements / 3; i++) {
    for (int c = 0; c < 3; c++) {
      float val = data[c * (img->shape[1] * img->shape[2]) + i];
      unsigned char pixel = (unsigned char)((val + 1.0f) * 127.5f);
      fwrite(&pixel, 1, 1, f);
    }
  }
  fclose(f);
  free(cpath);
}

void io_mkdirs(ZString path) {
  char *cpath = zstr_to_c(path);
  mkdir(cpath, 0777);
  free(cpath);
}

void io_write_json(ZString path, ZString json) {
  char *cpath = zstr_to_c(path);
  FILE *f = fopen(cpath, "w");
  if (!f)
    return;
  fwrite(json.ptr, 1, json.len, f);
  fclose(f);
  free(cpath);
}

// ============================================
// VISION FUNCTIONS
// ============================================

NovaTensor *vision_face_crop(NovaContext *ctx, NovaTensor *img,
                               int target) {
  // Stub: just resize for now
  Image *i = (Image *)malloc(sizeof(Image));
  i->width = (int)img->shape[2];
  i->height = (int)img->shape[1];
  i->channels = 3;
  i->data = (uint8_t *)malloc(i->width * i->height * 3);

  float *img_data = (float *)img->data;
  // Copy and denormalize
  for (int idx = 0; idx < i->width * i->height; idx++) {
    for (int c = 0; c < 3; c++) {
      float v = img_data[c * (i->width * i->height) + idx];
      i->data[idx * 3 + c] = (uint8_t)((v + 1.0f) * 127.5f);
    }
  }
  Image *resized = resize_image(i, target, target);
  NovaTensor *out = image_to_tensor(ctx, resized);
  free_image(i);
  free_image(resized);
  return out;
}

NovaTensor *vision_bg_remove(NovaTensor *img) { return img; } // Stub
NovaTensor *vision_alpha_premultiply(NovaTensor *img) {
  return img;
} // Stub
NovaTensor *vision_resize(NovaContext *ctx, NovaTensor *img, int w,
                            int h) {
  // Use the same logic as face_crop
  return vision_face_crop(ctx, img, w);
}

// ============================================
// STABLE DIFFUSION FUNCTIONS
// ============================================

void *sd_load_unet(ZString model_id) {
  (void)model_id;
  // Hardcode SD 1.5 path for this demo
  return load_sd15_unet("../../stable-diffusion-nova/models/Stable-diffusion/"
                        "v1-5-pruned-emaonly.safetensors");
}

void *sd_load_vae(ZString model_id) {
  (void)model_id;
  return NULL;
}
void *sd_load_clip(ZString model_id) {
  (void)model_id;
  return NULL;
}

NovaTensor *sd_encode_prompt(NovaContext *ctx, void *clip, ZString text,
                               ZString negative, int clip_skip) {
  (void)clip;
  (void)text;
  (void)negative;
  (void)clip_skip;
  int64_t shape[2] = {1, 768};
  return nova_tensor_create(ctx, shape, 2, NOVA_DTYPE_FP32); // Stub
}

NovaTensor *sd_sample_latents(NovaContext *ctx, void *unet, void *vae,
                                NovaTensor *cond, int seed, int steps,
                                float cfg, ZString sampler, int w, int h,
                                float denoise) {
  (void)unet;
  (void)vae;
  (void)cond;
  (void)seed;
  (void)steps;
  (void)cfg;
  (void)sampler;
  (void)denoise;
  int64_t shape[3] = {4, h / 8, w / 8};
  return nova_tensor_create(ctx, shape, 3, NOVA_DTYPE_FP32); // Stub
}

NovaTensor *sd_decode(NovaContext *ctx, void *vae, NovaTensor *latents) {
  (void)vae;
  int64_t shape[3] = {3, latents->shape[1] * 8, latents->shape[2] * 8};
  return nova_tensor_create(ctx, shape, 3, NOVA_DTYPE_FP32); // Stub
}

// =lora
void *lora_load(ZString path) {
  (void)path;
  return NULL;
}
void *lora_apply(void *unet, void *clip, void **loras, float *weights) {
  (void)clip;
  (void)loras;
  (void)weights;
  return unet;
}

// =pack
char *pack_telegram_stickers_from_pngs(ZString dir, ZString zip,
                                       ZString title) {
  (void)dir;
  (void)zip;
  (void)title;
  return "stickers.zip";
}
char *pack_telegram_custom_emojis_from_pngs(ZString dir, ZString zip,
                                            ZString title) {
  (void)dir;
  (void)zip;
  (void)title;
  return "emojis.zip";
}
