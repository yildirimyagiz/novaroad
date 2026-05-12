/**
 * @file event_loop.c
 * @brief Event loop implementation
 */

#include "runtime/async.h"
#include "std/alloc.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/epoll.h>
#define HAVE_EPOLL 1
#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <sys/event.h>
#define HAVE_KQUEUE 1
#else
#include <poll.h>
#define HAVE_POLL 1
#endif

#define MAX_EVENTS 1024
#define TIMER_RESOLUTION_MS 1

typedef struct nova_timer {
    uint64_t id;
    uint64_t timeout_ms;
    uint64_t repeat_ms;
    nova_timer_callback_t callback;
    void *user_data;
    struct nova_timer *next;
} nova_timer_t;

typedef struct nova_io_watcher {
    int fd;
    int events;
    nova_io_callback_t callback;
    void *user_data;
    struct nova_io_watcher *next;
} nova_io_watcher_t;

typedef struct nova_signal_watcher {
    int signum;
    nova_signal_callback_t callback;
    void *user_data;
    struct nova_signal_watcher *next;
} nova_signal_watcher_t;

struct nova_event_loop {
    bool running;
    bool stop_requested;

    // Platform-specific event handling
#ifdef HAVE_EPOLL
    int epoll_fd;
    struct epoll_event events[MAX_EVENTS];
#elif defined(HAVE_KQUEUE)
    int kqueue_fd;
    struct kevent events[MAX_EVENTS];
#else
    struct pollfd poll_fds[MAX_EVENTS];
    nova_io_watcher_t *poll_watchers[MAX_EVENTS];
    nfds_t poll_count;
#endif

    // Timers
    nova_timer_t *timers;
    uint64_t next_timer_id;

    // IO watchers
    nova_io_watcher_t *io_watchers;

    // Signal watchers
    nova_signal_watcher_t *signal_watchers;

    // Idle callbacks
    nova_idle_callback_t idle_callback;
    void *idle_user_data;

    // Prepare callbacks
    nova_prepare_callback_t prepare_callback;
    void *prepare_user_data;

    // Check callbacks
    nova_check_callback_t check_callback;
    void *check_user_data;
};

static uint64_t get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t) ts.tv_sec * 1000 + (uint64_t) ts.tv_nsec / 1000000;
}

nova_event_loop_t *nova_event_loop_create(void)
{
    nova_event_loop_t *loop = nova_alloc(sizeof(nova_event_loop_t));
    if (!loop) {
        return NULL;
    }

    memset(loop, 0, sizeof(nova_event_loop_t));

#ifdef HAVE_EPOLL
    loop->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (loop->epoll_fd == -1) {
        nova_free(loop);
        return NULL;
    }
#elif defined(HAVE_KQUEUE)
    loop->kqueue_fd = kqueue();
    if (loop->kqueue_fd == -1) {
        nova_free(loop);
        return NULL;
    }
#endif

    return loop;
}

void nova_event_loop_destroy(nova_event_loop_t *loop)
{
    if (!loop)
        return;

    // Clean up timers
    nova_timer_t *timer = loop->timers;
    while (timer) {
        nova_timer_t *next = timer->next;
        nova_free(timer);
        timer = next;
    }

    // Clean up IO watchers
    nova_io_watcher_t *io_watcher = loop->io_watchers;
    while (io_watcher) {
        nova_io_watcher_t *next = io_watcher->next;
        nova_free(io_watcher);
        io_watcher = next;
    }

    // Clean up signal watchers
    nova_signal_watcher_t *signal_watcher = loop->signal_watchers;
    while (signal_watcher) {
        nova_signal_watcher_t *next = signal_watcher->next;
        nova_free(signal_watcher);
        signal_watcher = next;
    }

#ifdef HAVE_EPOLL
    if (loop->epoll_fd != -1) {
        close(loop->epoll_fd);
    }
#elif defined(HAVE_KQUEUE)
    if (loop->kqueue_fd != -1) {
        close(loop->kqueue_fd);
    }
#endif

    nova_free(loop);
}

static void process_timers(nova_event_loop_t *loop)
{
    uint64_t now = get_time_ms();
    nova_timer_t *timer = loop->timers;
    nova_timer_t *prev = NULL;

    while (timer) {
        if (now >= timer->timeout_ms) {
            // Timer expired, call callback
            timer->callback(loop, timer->id, timer->user_data);

            if (timer->repeat_ms > 0) {
                // Repeating timer, reschedule
                timer->timeout_ms = now + timer->repeat_ms;
                prev = timer;
                timer = timer->next;
            } else {
                // One-shot timer, remove
                if (prev) {
                    prev->next = timer->next;
                } else {
                    loop->timers = timer->next;
                }
                nova_timer_t *to_free = timer;
                timer = timer->next;
                nova_free(to_free);
            }
        } else {
            prev = timer;
            timer = timer->next;
        }
    }
}

static int calculate_timeout(nova_event_loop_t *loop)
{
    if (!loop->timers) {
        return -1; // No timers, block indefinitely
    }

    uint64_t now = get_time_ms();
    uint64_t next_timeout = loop->timers->timeout_ms;

    nova_timer_t *timer = loop->timers->next;
    while (timer) {
        if (timer->timeout_ms < next_timeout) {
            next_timeout = timer->timeout_ms;
        }
        timer = timer->next;
    }

    if (next_timeout <= now) {
        return 0; // Timer already expired
    }

    uint64_t diff = next_timeout - now;
    return (int) (diff < INT_MAX ? diff : INT_MAX);
}

#ifdef HAVE_EPOLL
static void process_io_events_epoll(nova_event_loop_t *loop, int timeout_ms)
{
    int nfds = epoll_wait(loop->epoll_fd, loop->events, MAX_EVENTS, timeout_ms);
    if (nfds == -1) {
        if (errno != EINTR) {
            perror("epoll_wait");
        }
        return;
    }

    for (int i = 0; i < nfds; i++) {
        struct epoll_event *event = &loop->events[i];
        nova_io_watcher_t *watcher = (nova_io_watcher_t *) event->data.ptr;

        int events = 0;
        if (event->events & EPOLLIN)
            events |= NOVA_IO_READ;
        if (event->events & EPOLLOUT)
            events |= NOVA_IO_WRITE;
        if (event->events & EPOLLERR)
            events |= NOVA_IO_ERROR;

        watcher->callback(loop, watcher->fd, events, watcher->user_data);
    }
}

static int add_io_watcher_epoll(nova_event_loop_t *loop, nova_io_watcher_t *watcher)
{
    struct epoll_event event;
    event.data.ptr = watcher;
    event.events = EPOLLET; // Edge-triggered

    if (watcher->events & NOVA_IO_READ)
        event.events |= EPOLLIN;
    if (watcher->events & NOVA_IO_WRITE)
        event.events |= EPOLLOUT;

    return epoll_ctl(loop->epoll_fd, EPOLL_CTL_ADD, watcher->fd, &event);
}

static int remove_io_watcher_epoll(nova_event_loop_t *loop, nova_io_watcher_t *watcher)
{
    return epoll_ctl(loop->epoll_fd, EPOLL_CTL_DEL, watcher->fd, NULL);
}

#elif defined(HAVE_KQUEUE)
static void process_io_events_kqueue(nova_event_loop_t *loop, int timeout_ms)
{
    struct timespec timeout;
    if (timeout_ms >= 0) {
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_nsec = (timeout_ms % 1000) * 1000000;
    }

    int nevents = kevent(loop->kqueue_fd, NULL, 0, loop->events, MAX_EVENTS,
                         timeout_ms >= 0 ? &timeout : NULL);
    if (nevents == -1) {
        if (errno != EINTR) {
            perror("kevent");
        }
        return;
    }

    for (int i = 0; i < nevents; i++) {
        struct kevent *event = &loop->events[i];
        nova_io_watcher_t *watcher = (nova_io_watcher_t *) event->udata;

        int events = 0;
        if (event->filter == EVFILT_READ)
            events |= NOVA_IO_READ;
        if (event->filter == EVFILT_WRITE)
            events |= NOVA_IO_WRITE;
        if (event->flags & EV_ERROR)
            events |= NOVA_IO_ERROR;

        watcher->callback(loop, watcher->fd, events, watcher->user_data);
    }
}

static int add_io_watcher_kqueue(nova_event_loop_t *loop, nova_io_watcher_t *watcher)
{
    struct kevent changes[2];
    int nchanges = 0;

    if (watcher->events & NOVA_IO_READ) {
        EV_SET(&changes[nchanges], watcher->fd, EVFILT_READ, EV_ADD, 0, 0, watcher);
        nchanges++;
    }
    if (watcher->events & NOVA_IO_WRITE) {
        EV_SET(&changes[nchanges], watcher->fd, EVFILT_WRITE, EV_ADD, 0, 0, watcher);
        nchanges++;
    }

    return kevent(loop->kqueue_fd, changes, nchanges, NULL, 0, NULL);
}

static int remove_io_watcher_kqueue(nova_event_loop_t *loop, nova_io_watcher_t *watcher)
{
    struct kevent changes[2];
    int nchanges = 0;

    if (watcher->events & NOVA_IO_READ) {
        EV_SET(&changes[nchanges], watcher->fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
        nchanges++;
    }
    if (watcher->events & NOVA_IO_WRITE) {
        EV_SET(&changes[nchanges], watcher->fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
        nchanges++;
    }

    return kevent(loop->kqueue_fd, changes, nchanges, NULL, 0, NULL);
}

#else // HAVE_POLL
static void process_io_events_poll(nova_event_loop_t *loop, int timeout_ms)
{
    int ret = poll(loop->poll_fds, loop->poll_count, timeout_ms);
    if (ret == -1) {
        if (errno != EINTR) {
            perror("poll");
        }
        return;
    }

    for (nfds_t i = 0; i < loop->poll_count; i++) {
        struct pollfd *pfd = &loop->poll_fds[i];
        nova_io_watcher_t *watcher = loop->poll_watchers[i];

        if (pfd->revents == 0)
            continue;

        int events = 0;
        if (pfd->revents & POLLIN)
            events |= NOVA_IO_READ;
        if (pfd->revents & POLLOUT)
            events |= NOVA_IO_WRITE;
        if (pfd->revents & (POLLERR | POLLHUP | POLLNVAL))
            events |= NOVA_IO_ERROR;

        watcher->callback(loop, watcher->fd, events, watcher->user_data);
    }
}

static void rebuild_poll_fds(nova_event_loop_t *loop)
{
    loop->poll_count = 0;
    nova_io_watcher_t *watcher = loop->io_watchers;

    while (watcher && loop->poll_count < MAX_EVENTS) {
        loop->poll_fds[loop->poll_count].fd = watcher->fd;
        loop->poll_fds[loop->poll_count].events = 0;

        if (watcher->events & NOVA_IO_READ)
            loop->poll_fds[loop->poll_count].events |= POLLIN;
        if (watcher->events & NOVA_IO_WRITE)
            loop->poll_fds[loop->poll_count].events |= POLLOUT;

        loop->poll_watchers[loop->poll_count] = watcher;
        loop->poll_count++;
        watcher = watcher->next;
    }
}

static int add_io_watcher_poll(nova_event_loop_t *loop, nova_io_watcher_t *watcher)
{
    // Just add to list, poll fds will be rebuilt on next iteration
    watcher->next = loop->io_watchers;
    loop->io_watchers = watcher;
    return 0;
}

static int remove_io_watcher_poll(nova_event_loop_t *loop, nova_io_watcher_t *watcher)
{
    nova_io_watcher_t *prev = NULL;
    nova_io_watcher_t *curr = loop->io_watchers;

    while (curr) {
        if (curr == watcher) {
            if (prev) {
                prev->next = curr->next;
            } else {
                loop->io_watchers = curr->next;
            }
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    return -1;
}
#endif

static void process_signals(nova_event_loop_t *loop)
{
    // Process signal watchers
    nova_signal_watcher_t *watcher = loop->signal_watchers;
    while (watcher) {
        // In a real implementation, we'd check for pending signals
        // For now, this is a placeholder
        watcher = watcher->next;
    }
}

void nova_event_loop_run(nova_event_loop_t *loop)
{
    loop->running = true;
    loop->stop_requested = false;

    while (loop->running && !loop->stop_requested) {
        // Prepare phase
        if (loop->prepare_callback) {
            loop->prepare_callback(loop, loop->prepare_user_data);
        }

        // Process timers
        process_timers(loop);

        // Calculate timeout for IO multiplexing
        int timeout = calculate_timeout(loop);

        // IO multiplexing
#ifdef HAVE_EPOLL
        process_io_events_epoll(loop, timeout);
        rebuild_poll_fds(loop); // In case watchers changed
#elif defined(HAVE_KQUEUE)
        process_io_events_kqueue(loop, timeout);
#else
        rebuild_poll_fds(loop);
        process_io_events_poll(loop, timeout);
#endif

        // Process signals
        process_signals(loop);

        // Idle phase
        if (timeout == -1 && loop->idle_callback) {
            loop->idle_callback(loop, loop->idle_user_data);
        }

        // Check phase
        if (loop->check_callback) {
            loop->check_callback(loop, loop->check_user_data);
        }
    }

    loop->running = false;
}

void nova_event_loop_stop(nova_event_loop_t *loop)
{
    loop->stop_requested = true;
}

uint64_t nova_event_loop_timer(nova_event_loop_t *loop, uint64_t timeout_ms, uint64_t repeat_ms,
                               nova_timer_callback_t callback, void *user_data)
{
    nova_timer_t *timer = nova_alloc(sizeof(nova_timer_t));
    if (!timer)
        return 0;

    timer->id = ++loop->next_timer_id;
    timer->timeout_ms = get_time_ms() + timeout_ms;
    timer->repeat_ms = repeat_ms;
    timer->callback = callback;
    timer->user_data = user_data;

    // Insert at head (simplified - should be sorted by timeout)
    timer->next = loop->timers;
    loop->timers = timer;

    return timer->id;
}

void nova_event_loop_timer_stop(nova_event_loop_t *loop, uint64_t timer_id)
{
    nova_timer_t *timer = loop->timers;
    nova_timer_t *prev = NULL;

    while (timer) {
        if (timer->id == timer_id) {
            if (prev) {
                prev->next = timer->next;
            } else {
                loop->timers = timer->next;
            }
            nova_free(timer);
            return;
        }
        prev = timer;
        timer = timer->next;
    }
}

int nova_event_loop_io(nova_event_loop_t *loop, int fd, int events, nova_io_callback_t callback,
                       void *user_data)
{
    nova_io_watcher_t *watcher = nova_alloc(sizeof(nova_io_watcher_t));
    if (!watcher)
        return -1;

    watcher->fd = fd;
    watcher->events = events;
    watcher->callback = callback;
    watcher->user_data = user_data;

#ifdef HAVE_EPOLL
    if (add_io_watcher_epoll(loop, watcher) == -1) {
        nova_free(watcher);
        return -1;
    }
#elif defined(HAVE_KQUEUE)
    if (add_io_watcher_kqueue(loop, watcher) == -1) {
        nova_free(watcher);
        return -1;
    }
#else
    if (add_io_watcher_poll(loop, watcher) == -1) {
        nova_free(watcher);
        return -1;
    }
#endif

    return 0;
}

void nova_event_loop_io_stop(nova_event_loop_t *loop, int fd)
{
    nova_io_watcher_t *watcher = loop->io_watchers;
    nova_io_watcher_t *prev = NULL;

    while (watcher) {
        if (watcher->fd == fd) {
#ifdef HAVE_EPOLL
            remove_io_watcher_epoll(loop, watcher);
#elif defined(HAVE_KQUEUE)
            remove_io_watcher_kqueue(loop, watcher);
#else
            remove_io_watcher_poll(loop, watcher);
#endif

            if (prev) {
                prev->next = watcher->next;
            } else {
                loop->io_watchers = watcher->next;
            }
            nova_free(watcher);
            return;
        }
        prev = watcher;
        watcher = watcher->next;
    }
}

void nova_event_loop_set_idle_callback(nova_event_loop_t *loop, nova_idle_callback_t callback,
                                       void *user_data)
{
    loop->idle_callback = callback;
    loop->idle_user_data = user_data;
}

void nova_event_loop_set_prepare_callback(nova_event_loop_t *loop, nova_prepare_callback_t callback,
                                          void *user_data)
{
    loop->prepare_callback = callback;
    loop->prepare_user_data = user_data;
}

void nova_event_loop_set_check_callback(nova_event_loop_t *loop, nova_check_callback_t callback,
                                        void *user_data)
{
    loop->check_callback = callback;
    loop->check_user_data = user_data;
}
