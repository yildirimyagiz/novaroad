/**
 * FULL WORKING MOBILE APP - Nova Todo Application
 * Demonstrates complete mobile app logic with UI simulation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// ════════════════════════════════════════════════════════════════════════════
// DATA STRUCTURES
// ════════════════════════════════════════════════════════════════════════════

typedef struct {
    int id;
    char title[100];
    bool completed;
} Todo;

typedef struct {
    Todo todos[100];
    int count;
    int next_id;
} AppState;

// ════════════════════════════════════════════════════════════════════════════
// APP LOGIC
// ════════════════════════════════════════════════════════════════════════════

void init_app(AppState* app) {
    app->count = 0;
    app->next_id = 1;
}

void add_todo(AppState* app, const char* title) {
    if (app->count >= 100) return;
    
    Todo todo;
    todo.id = app->next_id++;
    strncpy(todo.title, title, sizeof(todo.title) - 1);
    todo.completed = false;
    
    app->todos[app->count++] = todo;
}

void toggle_todo(AppState* app, int id) {
    for (int i = 0; i < app->count; i++) {
        if (app->todos[i].id == id) {
            app->todos[i].completed = !app->todos[i].completed;
            break;
        }
    }
}

void delete_todo(AppState* app, int id) {
    for (int i = 0; i < app->count; i++) {
        if (app->todos[i].id == id) {
            // Shift todos
            for (int j = i; j < app->count - 1; j++) {
                app->todos[j] = app->todos[j + 1];
            }
            app->count--;
            break;
        }
    }
}

int count_completed(AppState* app) {
    int count = 0;
    for (int i = 0; i < app->count; i++) {
        if (app->todos[i].completed) count++;
    }
    return count;
}

int count_active(AppState* app) {
    return app->count - count_completed(app);
}

// ════════════════════════════════════════════════════════════════════════════
// UI RENDERING
// ════════════════════════════════════════════════════════════════════════════

void render_todo(Todo* todo) {
    printf("  %s [%d] %s\n", 
           todo->completed ? "✅" : "⬜", 
           todo->id, 
           todo->title);
}

void render_app(AppState* app) {
    printf("\n");
    printf("┌──────────────────────────────────────────────────────────────┐\n");
    printf("│              📱 NOVA TODO APP                                │\n");
    printf("└──────────────────────────────────────────────────────────────┘\n");
    printf("\n");
    
    if (app->count == 0) {
        printf("  No todos yet. Add one to get started!\n");
    } else {
        for (int i = 0; i < app->count; i++) {
            render_todo(&app->todos[i]);
        }
    }
    
    printf("\n");
    printf("  📊 Total: %d | ✅ Completed: %d | ⬜ Active: %d\n", 
           app->count, count_completed(app), count_active(app));
    printf("\n");
}

// ════════════════════════════════════════════════════════════════════════════
// MAIN APP
// ════════════════════════════════════════════════════════════════════════════

int main() {
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                 📱 NOVA MOBILE TODO APP - RUNNING                            ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    
    AppState app;
    init_app(&app);
    
    // Initial state
    printf("\n🎬 APP STARTUP\n");
    render_app(&app);
    
    // Add todos
    printf("\n➕ ACTION: Add 'Learn Nova'\n");
    add_todo(&app, "Learn Nova");
    render_app(&app);
    
    printf("\n➕ ACTION: Add 'Build mobile app'\n");
    add_todo(&app, "Build mobile app");
    render_app(&app);
    
    printf("\n➕ ACTION: Add 'Deploy to App Store'\n");
    add_todo(&app, "Deploy to App Store");
    render_app(&app);
    
    // Complete first todo
    printf("\n✅ ACTION: Complete todo #1\n");
    toggle_todo(&app, 1);
    render_app(&app);
    
    // Complete second todo
    printf("\n✅ ACTION: Complete todo #2\n");
    toggle_todo(&app, 2);
    render_app(&app);
    
    // Toggle back
    printf("\n↩️  ACTION: Uncomplete todo #2\n");
    toggle_todo(&app, 2);
    render_app(&app);
    
    // Delete a todo
    printf("\n🗑️  ACTION: Delete todo #3\n");
    delete_todo(&app, 3);
    render_app(&app);
    
    // Final statistics
    printf("\n┌──────────────────────────────────────────────────────────────┐\n");
    printf("│              📊 SESSION STATISTICS                           │\n");
    printf("└──────────────────────────────────────────────────────────────┘\n");
    printf("\n");
    printf("  Actions performed: 7\n");
    printf("  Todos created: 3\n");
    printf("  Todos completed: 1\n");
    printf("  Todos deleted: 1\n");
    printf("  Final todo count: %d\n", app.count);
    printf("\n");
    
    printf("╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║            ✅ NOVA MOBILE TODO APP - DEMO COMPLETE!                          ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════════════╝\n");
    
    return 0;
}
