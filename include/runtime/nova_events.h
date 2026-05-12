/**
 * Nova Event System
 * Provides event handling and reactive state management
 */

#ifndef NOVA_EVENTS_H
#define NOVA_EVENTS_H

#include <stdbool.h>
#include <stddef.h>

// ═══════════════════════════════════════════════════════════════════════════
// EVENT TYPES
// ═══════════════════════════════════════════════════════════════════════════

typedef enum {
  EVENT_CLICK,
  EVENT_CHANGE,
  EVENT_INPUT,
  EVENT_SUBMIT,
  EVENT_KEYDOWN,
  EVENT_KEYUP,
  EVENT_MOUSEENTER,
  EVENT_MOUSELEAVE,
  EVENT_FOCUS,
  EVENT_BLUR,
  EVENT_CUSTOM
} NovaEventType;

// ═══════════════════════════════════════════════════════════════════════════
// EVENT STRUCTURES
// ═══════════════════════════════════════════════════════════════════════════

typedef struct {
  NovaEventType type;
  void *target;           // Element that triggered the event
  void *current_target;   // Element that handler is attached to
  char *custom_type;      // For custom events
  void *data;             // Event-specific data
  bool bubbles;
  bool cancelable;
  bool default_prevented;
} NovaEvent;

// Event handler callback signature
typedef void (*NovaEventHandler)(NovaEvent *event, void *user_data);

typedef struct NovaEventListener {
  NovaEventType type;
  char *custom_type;
  NovaEventHandler handler;
  void *user_data;
  bool use_capture;
  struct NovaEventListener *next;
} NovaEventListener;

// ═══════════════════════════════════════════════════════════════════════════
// REACTIVE STATE
// ═══════════════════════════════════════════════════════════════════════════

typedef struct NovaState NovaState;
typedef void (*NovaStateObserver)(void *new_value, void *old_value, void *user_data);

struct NovaState {
  void *value;
  size_t value_size;
  NovaStateObserver *observers;
  size_t observer_count;
  size_t observer_capacity;
  char *name;  // For debugging
};

// ═══════════════════════════════════════════════════════════════════════════
// EVENT API
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Add an event listener to an element
 */
void nova_add_event_listener(void *element, NovaEventType type, 
                                NovaEventHandler handler, void *user_data);

/**
 * Add a custom event listener
 */
void nova_add_custom_event_listener(void *element, const char *event_type,
                                       NovaEventHandler handler, void *user_data);

/**
 * Remove an event listener
 */
void nova_remove_event_listener(void *element, NovaEventType type,
                                   NovaEventHandler handler);

/**
 * Dispatch an event on an element
 */
bool nova_dispatch_event(void *element, NovaEvent *event);

/**
 * Create a new event
 */
NovaEvent *nova_create_event(NovaEventType type, void *target);

/**
 * Destroy an event
 */
void nova_destroy_event(NovaEvent *event);

/**
 * Prevent default behavior
 */
void nova_prevent_default(NovaEvent *event);

/**
 * Stop event propagation
 */
void nova_stop_propagation(NovaEvent *event);

// ═══════════════════════════════════════════════════════════════════════════
// REACTIVE STATE API
// ═══════════════════════════════════════════════════════════════════════════

/**
 * Create a new reactive state
 */
NovaState *nova_create_state(const char *name, void *initial_value, size_t value_size);

/**
 * Get the current value of a state
 */
void *nova_state_get(NovaState *state);

/**
 * Set a new value for a state (triggers observers)
 */
void nova_state_set(NovaState *state, void *new_value);

/**
 * Observe changes to a state
 */
void nova_state_observe(NovaState *state, NovaStateObserver observer, void *user_data);

/**
 * Remove an observer from a state
 */
void nova_state_unobserve(NovaState *state, NovaStateObserver observer);

/**
 * Destroy a state
 */
void nova_destroy_state(NovaState *state);

// ═══════════════════════════════════════════════════════════════════════════
// CONVENIENCE MACROS
// ═══════════════════════════════════════════════════════════════════════════

#define NOVA_STATE_INT(name, initial) \
  nova_create_state(name, &(int){initial}, sizeof(int))

#define NOVA_STATE_STRING(name, initial) \
  nova_create_state(name, initial, strlen(initial) + 1)

#define NOVA_GET_INT(state) \
  (*(int*)nova_state_get(state))

#define NOVA_SET_INT(state, value) \
  do { int _val = (value); nova_state_set(state, &_val); } while(0)

#endif // NOVA_EVENTS_H
