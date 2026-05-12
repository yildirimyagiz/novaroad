/**
 * Nova Runtime Telemetry Implementation
 */

#include "runtime/telemetry/nova_telemetry.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

// ═══════════════════════════════════════════════════════════════════════════
// TIME UTILITIES
// ═══════════════════════════════════════════════════════════════════════════

static uint64_t get_time_microseconds(void) {
#ifdef _WIN32
  LARGE_INTEGER freq, counter;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&counter);
  return (uint64_t)((counter.QuadPart * 1000000) / freq.QuadPart);
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
#endif
}

static char *generate_id(void) {
  static uint64_t counter = 0;
  char *id = (char *)malloc(32);
  snprintf(id, 32, "%016llx", (unsigned long long)counter++);
  return id;
}

// ═══════════════════════════════════════════════════════════════════════════
// LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════════

TelemetryContext *telemetry_create(void) {
  TelemetryContext *ctx = (TelemetryContext *)calloc(1, sizeof(TelemetryContext));
  if (!ctx) return NULL;
  
  ctx->metric_capacity = 64;
  ctx->metrics = (Metric **)calloc(ctx->metric_capacity, sizeof(Metric*));
  
  ctx->completed_trace_capacity = 256;
  ctx->completed_traces = (Span **)calloc(ctx->completed_trace_capacity, sizeof(Span*));
  
  ctx->sampling_rate = 1.0;  // 100% sampling by default
  ctx->export_enabled = false;
  ctx->start_time = get_time_microseconds();
  
  return ctx;
}

void telemetry_destroy(TelemetryContext *ctx) {
  if (!ctx) return;
  
  // Free metrics
  for (size_t i = 0; i < ctx->metric_count; i++) {
    Metric *m = ctx->metrics[i];
    free(m->name);
    free(m->description);
    
    if (m->type == METRIC_HISTOGRAM) {
      free(m->data.histogram.buckets);
      free(m->data.histogram.counts);
    } else if (m->type == METRIC_SUMMARY) {
      free(m->data.summary.values);
    }
    
    for (size_t j = 0; j < m->label_count; j++) {
      free(m->label_keys[j]);
      free(m->label_values[j]);
    }
    free(m->label_keys);
    free(m->label_values);
    free(m);
  }
  free(ctx->metrics);
  
  free(ctx->export_endpoint);
  free(ctx);
}

void telemetry_reset(TelemetryContext *ctx) {
  if (!ctx) return;
  
  for (size_t i = 0; i < ctx->metric_count; i++) {
    Metric *m = ctx->metrics[i];
    if (m->type == METRIC_COUNTER) {
      m->data.counter_value = 0;
    } else if (m->type == METRIC_GAUGE) {
      m->data.gauge_value = 0.0;
    }
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════

void telemetry_set_sampling_rate(TelemetryContext *ctx, double rate) {
  if (!ctx) return;
  ctx->sampling_rate = rate < 0.0 ? 0.0 : (rate > 1.0 ? 1.0 : rate);
}

void telemetry_enable_export(TelemetryContext *ctx, const char *endpoint) {
  if (!ctx) return;
  ctx->export_enabled = true;
  if (ctx->export_endpoint) free(ctx->export_endpoint);
  ctx->export_endpoint = endpoint ? strdup(endpoint) : NULL;
}

void telemetry_disable_export(TelemetryContext *ctx) {
  if (!ctx) return;
  ctx->export_enabled = false;
}

// ═══════════════════════════════════════════════════════════════════════════
// METRICS - REGISTRATION
// ═══════════════════════════════════════════════════════════════════════════

static Metric *metric_create(const char *name, const char *description, MetricType type) {
  Metric *m = (Metric *)calloc(1, sizeof(Metric));
  m->name = strdup(name);
  m->description = description ? strdup(description) : NULL;
  m->type = type;
  m->last_update_time = get_time_microseconds();
  return m;
}

Metric *telemetry_register_counter(TelemetryContext *ctx, const char *name, const char *description) {
  if (!ctx || !name) return NULL;
  
  Metric *m = metric_create(name, description, METRIC_COUNTER);
  m->data.counter_value = 0;
  
  if (ctx->metric_count >= ctx->metric_capacity) {
    ctx->metric_capacity *= 2;
    ctx->metrics = (Metric **)realloc(ctx->metrics, ctx->metric_capacity * sizeof(Metric*));
  }
  ctx->metrics[ctx->metric_count++] = m;
  
  return m;
}

Metric *telemetry_register_gauge(TelemetryContext *ctx, const char *name, const char *description) {
  if (!ctx || !name) return NULL;
  
  Metric *m = metric_create(name, description, METRIC_GAUGE);
  m->data.gauge_value = 0.0;
  
  if (ctx->metric_count >= ctx->metric_capacity) {
    ctx->metric_capacity *= 2;
    ctx->metrics = (Metric **)realloc(ctx->metrics, ctx->metric_capacity * sizeof(Metric*));
  }
  ctx->metrics[ctx->metric_count++] = m;
  
  return m;
}

Metric *telemetry_register_histogram(TelemetryContext *ctx, const char *name, const char *description,
                                      double *buckets, size_t bucket_count) {
  if (!ctx || !name) return NULL;
  
  Metric *m = metric_create(name, description, METRIC_HISTOGRAM);
  m->data.histogram.bucket_count = bucket_count;
  m->data.histogram.buckets = (double *)malloc(bucket_count * sizeof(double));
  m->data.histogram.counts = (uint64_t *)calloc(bucket_count, sizeof(uint64_t));
  memcpy(m->data.histogram.buckets, buckets, bucket_count * sizeof(double));
  m->data.histogram.sum = 0;
  m->data.histogram.count = 0;
  
  if (ctx->metric_count >= ctx->metric_capacity) {
    ctx->metric_capacity *= 2;
    ctx->metrics = (Metric **)realloc(ctx->metrics, ctx->metric_capacity * sizeof(Metric*));
  }
  ctx->metrics[ctx->metric_count++] = m;
  
  return m;
}

Metric *telemetry_get_metric(TelemetryContext *ctx, const char *name) {
  if (!ctx || !name) return NULL;
  
  for (size_t i = 0; i < ctx->metric_count; i++) {
    if (strcmp(ctx->metrics[i]->name, name) == 0) {
      return ctx->metrics[i];
    }
  }
  return NULL;
}

// ═══════════════════════════════════════════════════════════════════════════
// METRICS - COUNTER
// ═══════════════════════════════════════════════════════════════════════════

void telemetry_counter_inc(Metric *metric) {
  if (!metric || metric->type != METRIC_COUNTER) return;
  metric->data.counter_value++;
  metric->last_update_time = get_time_microseconds();
}

void telemetry_counter_add(Metric *metric, uint64_t value) {
  if (!metric || metric->type != METRIC_COUNTER) return;
  metric->data.counter_value += value;
  metric->last_update_time = get_time_microseconds();
}

uint64_t telemetry_counter_get(const Metric *metric) {
  if (!metric || metric->type != METRIC_COUNTER) return 0;
  return metric->data.counter_value;
}

// ═══════════════════════════════════════════════════════════════════════════
// METRICS - GAUGE
// ═══════════════════════════════════════════════════════════════════════════

void telemetry_gauge_set(Metric *metric, double value) {
  if (!metric || metric->type != METRIC_GAUGE) return;
  metric->data.gauge_value = value;
  metric->last_update_time = get_time_microseconds();
}

void telemetry_gauge_inc(Metric *metric) {
  telemetry_gauge_add(metric, 1.0);
}

void telemetry_gauge_dec(Metric *metric) {
  telemetry_gauge_sub(metric, 1.0);
}

void telemetry_gauge_add(Metric *metric, double value) {
  if (!metric || metric->type != METRIC_GAUGE) return;
  metric->data.gauge_value += value;
  metric->last_update_time = get_time_microseconds();
}

void telemetry_gauge_sub(Metric *metric, double value) {
  telemetry_gauge_add(metric, -value);
}

double telemetry_gauge_get(const Metric *metric) {
  if (!metric || metric->type != METRIC_GAUGE) return 0.0;
  return metric->data.gauge_value;
}

// ═══════════════════════════════════════════════════════════════════════════
// METRICS - HISTOGRAM
// ═══════════════════════════════════════════════════════════════════════════

void telemetry_histogram_observe(Metric *metric, double value) {
  if (!metric || metric->type != METRIC_HISTOGRAM) return;
  
  metric->data.histogram.sum += (uint64_t)value;
  metric->data.histogram.count++;
  
  for (size_t i = 0; i < metric->data.histogram.bucket_count; i++) {
    if (value <= metric->data.histogram.buckets[i]) {
      metric->data.histogram.counts[i]++;
    }
  }
  
  metric->last_update_time = get_time_microseconds();
}

uint64_t telemetry_histogram_count(const Metric *metric) {
  if (!metric || metric->type != METRIC_HISTOGRAM) return 0;
  return metric->data.histogram.count;
}

double telemetry_histogram_sum(const Metric *metric) {
  if (!metric || metric->type != METRIC_HISTOGRAM) return 0.0;
  return (double)metric->data.histogram.sum;
}

double telemetry_histogram_mean(const Metric *metric) {
  if (!metric || metric->type != METRIC_HISTOGRAM || metric->data.histogram.count == 0) return 0.0;
  return (double)metric->data.histogram.sum / metric->data.histogram.count;
}

void telemetry_metric_add_label(Metric *metric, const char *key, const char *value) {
  if (!metric || !key || !value) return;
  
  metric->label_keys = (char **)realloc(metric->label_keys, (metric->label_count + 1) * sizeof(char*));
  metric->label_values = (char **)realloc(metric->label_values, (metric->label_count + 1) * sizeof(char*));
  
  metric->label_keys[metric->label_count] = strdup(key);
  metric->label_values[metric->label_count] = strdup(value);
  metric->label_count++;
}

// ═══════════════════════════════════════════════════════════════════════════
// TRACING - SPAN LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════════

Span *telemetry_start_span(TelemetryContext *ctx, const char *operation_name) {
  if (!ctx || !operation_name) return NULL;
  
  // Check sampling
  if (ctx->sampling_rate < 1.0) {
    double random = (double)rand() / RAND_MAX;
    if (random > ctx->sampling_rate) {
      return NULL;  // Skip this span
    }
  }
  
  Span *span = (Span *)calloc(1, sizeof(Span));
  span->trace_id = generate_id();
  span->span_id = generate_id();
  span->parent_id = NULL;
  span->operation_name = strdup(operation_name);
  span->start_time_us = get_time_microseconds();
  
  ctx->total_spans_created++;
  
  return span;
}

Span *telemetry_start_child_span(TelemetryContext *ctx, Span *parent, const char *operation_name) {
  if (!ctx || !parent || !operation_name) return NULL;
  
  Span *span = telemetry_start_span(ctx, operation_name);
  if (!span) return NULL;
  
  free(span->trace_id);
  span->trace_id = strdup(parent->trace_id);
  span->parent_id = strdup(parent->span_id);
  
  return span;
}

void telemetry_end_span(TelemetryContext *ctx, Span *span) {
  if (!ctx || !span) return;
  
  span->end_time_us = get_time_microseconds();
  span->duration_us = span->end_time_us - span->start_time_us;
  
  // Store completed span
  if (ctx->completed_trace_count >= ctx->completed_trace_capacity) {
    ctx->completed_trace_capacity *= 2;
    ctx->completed_traces = (Span **)realloc(ctx->completed_traces,
                                               ctx->completed_trace_capacity * sizeof(Span*));
  }
  ctx->completed_traces[ctx->completed_trace_count++] = span;
}

void telemetry_span_add_tag(Span *span, const char *key, const char *value) {
  if (!span || !key || !value) return;
  
  span->tag_keys = (char **)realloc(span->tag_keys, (span->tag_count + 1) * sizeof(char*));
  span->tag_values = (char **)realloc(span->tag_values, (span->tag_count + 1) * sizeof(char*));
  
  span->tag_keys[span->tag_count] = strdup(key);
  span->tag_values[span->tag_count] = strdup(value);
  span->tag_count++;
}

void telemetry_span_log(Span *span, const char *message) {
  if (!span || !message) return;
  
  span->logs = (char **)realloc(span->logs, (span->log_count + 1) * sizeof(char*));
  span->log_times = (uint64_t *)realloc(span->log_times, (span->log_count + 1) * sizeof(uint64_t));
  
  span->logs[span->log_count] = strdup(message);
  span->log_times[span->log_count] = get_time_microseconds();
  span->log_count++;
}

void telemetry_span_set_error(Span *span, const char *error) {
  if (!span) return;
  telemetry_span_add_tag(span, "error", "true");
  telemetry_span_log(span, error);
}

// ═══════════════════════════════════════════════════════════════════════════
// COMMON METRICS
// ═══════════════════════════════════════════════════════════════════════════

CommonMetrics *telemetry_register_common_metrics(TelemetryContext *ctx) {
  if (!ctx) return NULL;
  
  CommonMetrics *cm = (CommonMetrics *)calloc(1, sizeof(CommonMetrics));
  
  cm->memory_allocated = telemetry_register_counter(ctx, "memory_allocated_bytes", "Total memory allocated");
  cm->memory_freed = telemetry_register_counter(ctx, "memory_freed_bytes", "Total memory freed");
  cm->memory_current = telemetry_register_gauge(ctx, "memory_current_bytes", "Current memory usage");
  cm->gc_collections = telemetry_register_counter(ctx, "gc_collections_total", "Total GC collections");
  cm->gc_time_ms = telemetry_register_counter(ctx, "gc_time_milliseconds", "Total GC time");
  
  cm->cpu_time_user_ms = telemetry_register_counter(ctx, "cpu_time_user_milliseconds", "User CPU time");
  cm->cpu_time_system_ms = telemetry_register_counter(ctx, "cpu_time_system_milliseconds", "System CPU time");
  
  cm->compilations_total = telemetry_register_counter(ctx, "compilations_total", "Total compilations");
  cm->compilation_time_ms = telemetry_register_counter(ctx, "compilation_time_milliseconds", "Total compilation time");
  cm->compilation_errors = telemetry_register_counter(ctx, "compilation_errors_total", "Compilation errors");
  
  cm->function_calls = telemetry_register_counter(ctx, "function_calls_total", "Total function calls");
  cm->instruction_count = telemetry_register_counter(ctx, "instructions_total", "Total instructions executed");
  cm->execution_time_ms = telemetry_register_counter(ctx, "execution_time_milliseconds", "Total execution time");
  
  cm->file_reads = telemetry_register_counter(ctx, "file_reads_total", "Total file reads");
  cm->file_writes = telemetry_register_counter(ctx, "file_writes_total", "Total file writes");
  cm->io_bytes_read = telemetry_register_counter(ctx, "io_bytes_read_total", "Total bytes read");
  cm->io_bytes_written = telemetry_register_counter(ctx, "io_bytes_written_total", "Total bytes written");
  
  return cm;
}

// ═══════════════════════════════════════════════════════════════════════════
// EXPORT & REPORTING
// ═══════════════════════════════════════════════════════════════════════════

char *telemetry_export_metrics(TelemetryContext *ctx, ExportFormat format) {
  if (!ctx) return NULL;
  
  size_t buffer_size = 4096;
  char *buffer = (char *)malloc(buffer_size);
  size_t offset = 0;
  
  if (format == EXPORT_FORMAT_JSON) {
    offset += snprintf(buffer + offset, buffer_size - offset, "{\"metrics\":[");
    
    for (size_t i = 0; i < ctx->metric_count; i++) {
      Metric *m = ctx->metrics[i];
      
      if (i > 0) offset += snprintf(buffer + offset, buffer_size - offset, ",");
      
      offset += snprintf(buffer + offset, buffer_size - offset,
                         "{\"name\":\"%s\",\"type\":", m->name);
      
      if (m->type == METRIC_COUNTER) {
        offset += snprintf(buffer + offset, buffer_size - offset,
                           "\"counter\",\"value\":%llu", 
                           (unsigned long long)m->data.counter_value);
      } else if (m->type == METRIC_GAUGE) {
        offset += snprintf(buffer + offset, buffer_size - offset,
                           "\"gauge\",\"value\":%f", m->data.gauge_value);
      }
      
      offset += snprintf(buffer + offset, buffer_size - offset, "}");
      
      if (buffer_size - offset < 512) {
        buffer_size *= 2;
        buffer = (char *)realloc(buffer, buffer_size);
      }
    }
    
    offset += snprintf(buffer + offset, buffer_size - offset, "]}");
    
  } else if (format == EXPORT_FORMAT_PROMETHEUS) {
    for (size_t i = 0; i < ctx->metric_count; i++) {
      Metric *m = ctx->metrics[i];
      
      if (m->type == METRIC_COUNTER) {
        offset += snprintf(buffer + offset, buffer_size - offset,
                           "# TYPE %s counter\n%s %llu\n",
                           m->name, m->name, (unsigned long long)m->data.counter_value);
      } else if (m->type == METRIC_GAUGE) {
        offset += snprintf(buffer + offset, buffer_size - offset,
                           "# TYPE %s gauge\n%s %f\n",
                           m->name, m->name, m->data.gauge_value);
      }
      
      if (buffer_size - offset < 512) {
        buffer_size *= 2;
        buffer = (char *)realloc(buffer, buffer_size);
      }
    }
  }
  
  return buffer;
}

bool telemetry_export_to_file(TelemetryContext *ctx, const char *filename, ExportFormat format) {
  if (!ctx || !filename) return false;
  
  char *content = telemetry_export_metrics(ctx, format);
  if (!content) return false;
  
  FILE *f = fopen(filename, "w");
  if (!f) {
    free(content);
    return false;
  }
  
  fputs(content, f);
  fclose(f);
  free(content);
  
  return true;
}

char *telemetry_export_traces(TelemetryContext *ctx, ExportFormat format) {
  if (!ctx) return NULL;
  
  size_t buffer_size = 8192;
  char *buffer = (char *)malloc(buffer_size);
  size_t offset = 0;
  
  offset += snprintf(buffer + offset, buffer_size - offset, "{\"traces\":[");
  
  for (size_t i = 0; i < ctx->completed_trace_count; i++) {
    Span *s = ctx->completed_traces[i];
    
    if (i > 0) offset += snprintf(buffer + offset, buffer_size - offset, ",");
    
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "{\"trace_id\":\"%s\",\"span_id\":\"%s\",\"operation\":\"%s\","
                       "\"duration_us\":%llu}",
                       s->trace_id, s->span_id, s->operation_name,
                       (unsigned long long)s->duration_us);
    
    if (buffer_size - offset < 512) {
      buffer_size *= 2;
      buffer = (char *)realloc(buffer, buffer_size);
    }
  }
  
  offset += snprintf(buffer + offset, buffer_size - offset, "]}");
  
  return buffer;
}

void telemetry_export_periodic(TelemetryContext *ctx) {
  if (!ctx || !ctx->export_enabled) return;
  
  uint64_t now = get_time_microseconds();
  if (now - ctx->last_export_time < ctx->export_interval_ms * 1000) {
    return;
  }
  
  // Export metrics and traces
  char *metrics = telemetry_export_metrics(ctx, EXPORT_FORMAT_JSON);
  char *traces = telemetry_export_traces(ctx, EXPORT_FORMAT_JSON);
  
  // In production, send to endpoint (HTTP POST, etc.)
  // For now, just write to files
  if (metrics) {
    telemetry_export_to_file(ctx, "metrics.json", EXPORT_FORMAT_JSON);
    free(metrics);
  }
  if (traces) {
    FILE *f = fopen("traces.json", "w");
    if (f) {
      fputs(traces, f);
      fclose(f);
    }
    free(traces);
  }
  
  ctx->last_export_time = now;
}

ProfileReport *telemetry_get_profile_report(TelemetryContext *ctx) {
  if (!ctx) return NULL;
  
  ProfileReport *report = (ProfileReport *)calloc(1, sizeof(ProfileReport));
  // Implementation would aggregate span data into profile entries
  
  return report;
}

void telemetry_profile_report_destroy(ProfileReport *report) {
  if (!report) return;
  free(report->entries);
  free(report);
}
