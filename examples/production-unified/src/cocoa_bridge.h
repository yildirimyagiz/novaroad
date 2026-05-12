// ╔══════════════════════════════════════════════════════════════════════════════╗
// ║  Nova Desktop — FFI Bridge Header                                         ║
// ║  C-compatible declarations for Cocoa bridge functions                     ║
// ╚══════════════════════════════════════════════════════════════════════════════╝

#ifndef NOVA_COCOA_BRIDGE_H
#define NOVA_COCOA_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// ── App Lifecycle ──────────────────────────────────────────────────────────
void nova_app_init(void);
void nova_app_run(void);
void nova_app_quit(void);

// ── Window Management ──────────────────────────────────────────────────────
void *nova_window_create(double width, double height, const char *title);
void nova_window_show(void *window_ptr);
void nova_window_set_title(void *window_ptr, const char *title);
void nova_window_set_size(void *window_ptr, double width, double height);

// ── Labels ─────────────────────────────────────────────────────────────────
void *nova_label_create(void *window_ptr, const char *text, double x, double y,
                        double w, double h);
void *nova_title_label_create(void *window_ptr, const char *text, double x,
                              double y, double w, double h);
void nova_label_set_text(void *label_ptr, const char *text);

// ── Button ─────────────────────────────────────────────────────────────────
typedef void (*nova_button_callback_t)(int button_id);
void *nova_button_create(void *window_ptr, const char *title, double x,
                         double y, double w, double h, int button_id,
                         nova_button_callback_t callback);

// ── Text Input ─────────────────────────────────────────────────────────────
void *nova_text_input_create(void *window_ptr, const char *placeholder,
                             double x, double y, double w, double h);
const char *nova_text_input_get_value(void *input_ptr);

// ── Panel (Rounded Rect) ──────────────────────────────────────────────────
void *nova_panel_create(void *window_ptr, double x, double y, double w,
                        double h, double r, double g, double b, double a,
                        double corner_radius);

// ── Progress Bar ──────────────────────────────────────────────────────────
void *nova_progress_create(void *window_ptr, double x, double y, double w,
                           double h);
void nova_progress_set_value(void *progress_ptr, double value);

// ── Timer ─────────────────────────────────────────────────────────────────
typedef void (*nova_timer_callback_t)(void *user_data);
void *nova_timer_create(double interval_seconds, nova_timer_callback_t callback,
                        void *user_data);
void nova_timer_stop(void *timer_ptr);

// ── Dialog ────────────────────────────────────────────────────────────────
int nova_alert(const char *title, const char *message);

#ifdef __cplusplus
}
#endif

#endif /* NOVA_COCOA_BRIDGE_H */
