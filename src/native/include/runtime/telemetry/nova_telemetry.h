/**
 * Nova Runtime Telemetry
 * Performance monitoring, profiling, and observability
 */

#ifndef NOVA_TELEMETRY_H
#define NOVA_TELEMETRY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// ═══════════════════════════════════════════════════════════════════════════
// METRIC TYPES
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  METRIC_COUNTER,      // Monotonically increasing (e.g., requests_total)
  METRIC_GAUGE,        // Can go up or down (e.g., memory_usage)
  METRIC_HISTOGRAM,    // Distribution of values (e.g., latency)
  METRIC_SUMMARY,      // Quantiles over time window
} MetricType;

// ═══════════════════════════════════════════════════════════════════════════
// METRIC
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  char *name;
  char *description;
  MetricType type;
  
  union {
    uint64_t counter_value;
    double gauge_value;
    struct {
      double *buckets;
      uint64_t *counts;
      size_t bucket_count;
      uint64_t sum;
      uint64_t count;
    } histogram;
    struct {
      double *values;
      size_t value_count;
      size_t value_capacity;
    } summary;
  } data;
  
  // Labels (key-value pairs)
  char **label_keys;
  char **label_values;
  size_t label_count;
  
  uint64_t last_update_time;
} Metric;

// ═══════════════════════════════════════════════════════════════════════════
// SPAN (Distributed Tracing)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct Span {
  char *trace_id;      // Unique trace identifier
  char *span_id;       // Unique span identifier
  char *parent_id;     // Parent span ID (NULL for root)
  
  char *operation_name;
  uint64_t start_time_us;
  uint64_t end_time_us;
  uint64_t duration_us;
  
  // Tags
  char **tag_keys;
  char **tag_values;
  size_t tag_count;
  
  // Logs
  char **logs;
  uint64_t *log_times;
  size_t log_count;
  
  struct Span *children;
  size_t child_count;
} Span;

// ═══════════════════════════════════════════════════════════════════════════
// TELEMETRY CONTEXT
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  // Metrics registry
  Metric **metrics;
  size_t metric_count;
  size_t metric_capacity;
  
  // Active spans
  Span **active_spans;
  size_t active_span_count;
  
  // Completed traces
  Span **completed_traces;
  size_t completed_trace_count;
  size_t completed_trace_capacity;
  
  // Sampling
  double sampling_rate;  // 0.0 to 1.0 (1.0 = 100% sampling)
  
  // Export configuration
  bool export_enabled;
  char *export_endpoint;
  uint64_t export_interval_ms;
  uint64_t last_export_time;
  
  // Runtime stats
  uint64_t start_time;
  uint64_t total_spans_created;
  uint64_t total_metrics_updated;
} TelemetryContext;

// ═══════════════════════════════════════════════════════════════════════════
// TELEMETRY API
// ═══════════════════════════════════════════════════════════════════════════

// Lifecycle
TelemetryContext *telemetry_create(void);
void telemetry_destroy(TelemetryContext *ctx);
void telemetry_reset(TelemetryContext *ctx);

// Configuration
void telemetry_set_sampling_rate(TelemetryContext *ctx, double rate);
void telemetry_enable_export(TelemetryContext *ctx, const char *endpoint);
void telemetry_disable_export(TelemetryContext *ctx);

// ═══════════════════════════════════════════════════════════════════════════
// METRICS API
// ═══════════════════════════════════════════════════════════════════════════

// Metric registration
Metric *telemetry_register_counter(TelemetryContext *ctx, const char *name, const char *description);
Metric *telemetry_register_gauge(TelemetryContext *ctx, const char *name, const char *description);
Metric *telemetry_register_histogram(TelemetryContext *ctx, const char *name, const char *description,
                                      double *buckets, size_t bucket_count);

// Metric lookup
Metric *telemetry_get_metric(TelemetryContext *ctx, const char *name);

// Counter operations
void telemetry_counter_inc(Metric *metric);
void telemetry_counter_add(Metric *metric, uint64_t value);
uint64_t telemetry_counter_get(const Metric *metric);

// Gauge operations
void telemetry_gauge_set(Metric *metric, double value);
void telemetry_gauge_inc(Metric *metric);
void telemetry_gauge_dec(Metric *metric);
void telemetry_gauge_add(Metric *metric, double value);
void telemetry_gauge_sub(Metric *metric, double value);
double telemetry_gauge_get(const Metric *metric);

// Histogram operations
void telemetry_histogram_observe(Metric *metric, double value);
uint64_t telemetry_histogram_count(const Metric *metric);
double telemetry_histogram_sum(const Metric *metric);
double telemetry_histogram_mean(const Metric *metric);

// Metric labels
void telemetry_metric_add_label(Metric *metric, const char *key, const char *value);

// ═══════════════════════════════════════════════════════════════════════════
// TRACING API
// ═══════════════════════════════════════════════════════════════════════════

// Span lifecycle
Span *telemetry_start_span(TelemetryContext *ctx, const char *operation_name);
Span *telemetry_start_child_span(TelemetryContext *ctx, Span *parent, const char *operation_name);
void telemetry_end_span(TelemetryContext *ctx, Span *span);

// Span operations
void telemetry_span_add_tag(Span *span, const char *key, const char *value);
void telemetry_span_log(Span *span, const char *message);
void telemetry_span_set_error(Span *span, const char *error);

// Current span (thread-local)
Span *telemetry_current_span(TelemetryContext *ctx);
void telemetry_set_current_span(TelemetryContext *ctx, Span *span);

// ═══════════════════════════════════════════════════════════════════════════
// COMMON METRICS (Pre-defined)
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  // Memory metrics
  Metric *memory_allocated;
  Metric *memory_freed;
  Metric *memory_current;
  Metric *gc_collections;
  Metric *gc_time_ms;
  
  // CPU metrics
  Metric *cpu_time_user_ms;
  Metric *cpu_time_system_ms;
  
  // Compilation metrics
  Metric *compilations_total;
  Metric *compilation_time_ms;
  Metric *compilation_errors;
  
  // Execution metrics
  Metric *function_calls;
  Metric *instruction_count;
  Metric *execution_time_ms;
  
  // I/O metrics
  Metric *file_reads;
  Metric *file_writes;
  Metric *io_bytes_read;
  Metric *io_bytes_written;
} CommonMetrics;

CommonMetrics *telemetry_register_common_metrics(TelemetryContext *ctx);

// ═══════════════════════════════════════════════════════════════════════════
// EXPORT & REPORTING
// ═══════════════════════════════════════════════════════════════════════════

// Export formats
typedef enum {
  EXPORT_FORMAT_JSON,
  EXPORT_FORMAT_PROMETHEUS,
  EXPORT_FORMAT_OPENTELEMETRY,
} ExportFormat;

// Export metrics
char *telemetry_export_metrics(TelemetryContext *ctx, ExportFormat format);
bool telemetry_export_to_file(TelemetryContext *ctx, const char *filename, ExportFormat format);

// Export traces
char *telemetry_export_traces(TelemetryContext *ctx, ExportFormat format);

// Periodic export
void telemetry_export_periodic(TelemetryContext *ctx);

// ═══════════════════════════════════════════════════════════════════════════
// PROFILING
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  char *function_name;
  uint64_t call_count;
  uint64_t total_time_us;
  uint64_t min_time_us;
  uint64_t max_time_us;
  double avg_time_us;
} ProfileEntry;

typedef struct {
  ProfileEntry *entries;
  size_t entry_count;
  size_t entry_capacity;
} ProfileReport;

ProfileReport *telemetry_get_profile_report(TelemetryContext *ctx);
void telemetry_profile_report_destroy(ProfileReport *report);

// ═══════════════════════════════════════════════════════════════════════════
// MACROS FOR EASY INSTRUMENTATION
// ═══════════════════════════════════════════════════════════════════════════

#define TELEMETRY_SPAN_START(ctx, name) \
  Span *__span_##name = telemetry_start_span(ctx, #name)

#define TELEMETRY_SPAN_END(ctx, name) \
  telemetry_end_span(ctx, __span_##name)

#define TELEMETRY_COUNTER_INC(ctx, name) \
  do { \
    Metric *__m = telemetry_get_metric(ctx, name); \
    if (__m) telemetry_counter_inc(__m); \
  } while(0)

#define TELEMETRY_GAUGE_SET(ctx, name, value) \
  do { \
    Metric *__m = telemetry_get_metric(ctx, name); \
    if (__m) telemetry_gauge_set(__m, value); \
  } while(0)

#define TELEMETRY_HISTOGRAM_OBSERVE(ctx, name, value) \
  do { \
    Metric *__m = telemetry_get_metric(ctx, name); \
    if (__m) telemetry_histogram_observe(__m, value); \
  } while(0)

#endif // NOVA_TELEMETRY_H
