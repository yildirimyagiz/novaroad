/**
 * Nova Event System Implementation
 */

#include "nova_events.h"
#include "nova_web_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declaration of NovaElement from runtime
typedef struct NovaElement NovaElement;

// Extended element structure with event listeners
typedef struct {
  NovaElement *base;
  NovaEventListener *listeners;
} NovaEventElement;

// ═══════════════════════════════════════════════════════════════════════════
// EVENT MANAGEMENT
// ═══════════════════════════════════════════════════════════════════════════

void nova_add_event_listener(void *element, NovaEventType type, 
                                NovaEventHandler handler, void *user_data) {
  if (!element || !handler) return;

  NovaEventListener *listener = malloc(sizeof(NovaEventListener));
  if (!listener) return;

  listener->type = type;
  listener->custom_type = NULL;
  listener->handler = handler;
  listener->user_data = user_data;
  listener->use_capture = false;
  
  // Add to front of linked list (attached to element)
  // For simplicity, we're storing listeners in a global way
  // In real implementation, this would be attached to the element
  listener->next = NULL;
  
  // TODO: Attach to element properly
  free(listener); // Temporary - will be properly stored
}

void nova_add_custom_event_listener(void *element, const char *event_type,
                                       NovaEventHandler handler, void *user_data) {
  if (!element || !event_type || !handler) return;

  NovaEventListener *listener = malloc(sizeof(NovaEventListener));
  if (!listener) return;

  listener->type = EVENT_CUSTOM;
  listener->custom_type = strdup(event_type);
  listener->handler = handler;
  listener->user_data = user_data;
  listener->use_capture = false;
  listener->next = NULL;
  
  // TODO: Attach to element
  free(listener->custom_type);
  free(listener);
}

void nova_remove_event_listener(void *element, NovaEventType type,
                                   NovaEventHandler handler) {
  if (!element || !handler) return;
  // TODO: Implement removal from element's listener list
}

NovaEvent *nova_create_event(NovaEventType type, void *target) {
  NovaEvent *event = malloc(sizeof(NovaEvent));
  if (!event) return NULL;

  event->type = type;
  event->target = target;
  event->current_target = target;
  event->custom_type = NULL;
  event->data = NULL;
  event->bubbles = true;
  event->cancelable = true;
  event->default_prevented = false;

  return event;
}

void nova_destroy_event(NovaEvent *event) {
  if (!event) return;
  free(event->custom_type);
  free(event);
}

void nova_prevent_default(NovaEvent *event) {
  if (event && event->cancelable) {
    event->default_prevented = true;
  }
}

void nova_stop_propagation(NovaEvent *event) {
  if (event) {
    event->bubbles = false;
  }
}

bool nova_dispatch_event(void *element, NovaEvent *event) {
  if (!element || !event) return false;

  // Set current target
  event->current_target = element;

  // TODO: Call all matching event listeners
  // For now, just a placeholder
  
  return !event->default_prevented;
}

// ═══════════════════════════════════════════════════════════════════════════
// REACTIVE STATE SYSTEM
// ═══════════════════════════════════════════════════════════════════════════

NovaState *nova_create_state(const char *name, void *initial_value, size_t value_size) {
  NovaState *state = malloc(sizeof(NovaState));
  if (!state) return NULL;

  state->value = malloc(value_size);
  if (!state->value) {
    free(state);
    return NULL;
  }

  memcpy(state->value, initial_value, value_size);
  state->value_size = value_size;
  state->observers = NULL;
  state->observer_count = 0;
  state->observer_capacity = 0;
  state->name = name ? strdup(name) : NULL;

  return state;
}

void *nova_state_get(NovaState *state) {
  if (!state) return NULL;
  return state->value;
}

void nova_state_set(NovaState *state, void *new_value) {
  if (!state || !new_value) return;

  // Save old value for observers
  void *old_value = malloc(state->value_size);
  if (!old_value) return;
  memcpy(old_value, state->value, state->value_size);

  // Set new value
  memcpy(state->value, new_value, state->value_size);

  // Notify all observers
  for (size_t i = 0; i < state->observer_count; i++) {
    if (state->observers[i]) {
      state->observers[i](new_value, old_value, NULL);
    }
  }

  free(old_value);
}

void nova_state_observe(NovaState *state, NovaStateObserver observer, void *user_data) {
  if (!state || !observer) return;

  // Expand observer array if needed
  if (state->observer_count >= state->observer_capacity) {
    size_t new_capacity = state->observer_capacity == 0 ? 4 : state->observer_capacity * 2;
    NovaStateObserver *new_observers = realloc(state->observers, 
                                                   new_capacity * sizeof(NovaStateObserver));
    if (!new_observers) return;
    
    state->observers = new_observers;
    state->observer_capacity = new_capacity;
  }

  state->observers[state->observer_count++] = observer;
}

void nova_state_unobserve(NovaState *state, NovaStateObserver observer) {
  if (!state || !observer) return;

  for (size_t i = 0; i < state->observer_count; i++) {
    if (state->observers[i] == observer) {
      // Shift remaining observers
      for (size_t j = i; j < state->observer_count - 1; j++) {
        state->observers[j] = state->observers[j + 1];
      }
      state->observer_count--;
      return;
    }
  }
}

void nova_destroy_state(NovaState *state) {
  if (!state) return;

  free(state->value);
  free(state->observers);
  free(state->name);
  free(state);
}

// ═══════════════════════════════════════════════════════════════════════════
// CONVENIENCE FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

const char *nova_event_type_to_string(NovaEventType type) {
  switch (type) {
    case EVENT_CLICK: return "click";
    case EVENT_CHANGE: return "change";
    case EVENT_INPUT: return "input";
    case EVENT_SUBMIT: return "submit";
    case EVENT_KEYDOWN: return "keydown";
    case EVENT_KEYUP: return "keyup";
    case EVENT_MOUSEENTER: return "mouseenter";
    case EVENT_MOUSELEAVE: return "mouseleave";
    case EVENT_FOCUS: return "focus";
    case EVENT_BLUR: return "blur";
    case EVENT_CUSTOM: return "custom";
    default: return "unknown";
  }
}
