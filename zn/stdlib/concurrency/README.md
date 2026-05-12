# Nova Standard Library - Concurrency Module

The `concurrency` module provides high-level abstractions for concurrent and
parallel programming, including threads, async/await, channels, synchronization
primitives, and parallel algorithms.

## Architecture

```
concurrency/
├── async/               # Async runtime and futures
│   ├── future.zn       # Future trait and combinators
│   ├── async.zn        # async/await syntax support
│   └── executor.zn     # Async task executor
├── threads/             # Thread management
│   ├── thread.zn       # Thread creation and management
│   ├── join_handle.zn  # Thread join handles
│   └── thread_local.zn # Thread-local storage
├── sync/                # Synchronization primitives
│   ├── mutex.zn        # Mutual exclusion locks
│   ├── rwlock.zn       # Read-write locks
│   ├── semaphore.zn    # Counting semaphores
│   └── barrier.zn      # Synchronization barriers
├── channels/            # Message passing
│   ├── mpsc.zn         # Multi-producer, single-consumer
│   ├── spmc.zn         # Single-producer, multi-consumer
│   └── broadcast.zn    # Broadcast channels
└── parallel.zn          # Parallel algorithms and data structures
```

## Key Features

### Async/Await

- `async` functions and blocks
- `await` expressions for futures
- Future combinators (`join`, `select`, `race`)
- Async streams and iterators

### Thread Management

- Thread spawning and joining
- Thread pools with work stealing
- Thread-local storage
- CPU affinity control

### Synchronization

- Mutexes for exclusive access
- Read-write locks for shared access
- Atomic operations
- Condition variables

### Message Passing

- Channel-based communication
- Buffered and unbuffered channels
- Select statements for multiple channels
- Zero-copy message passing

### Parallel Algorithms

- Parallel iteration (`par_iter`)
- Parallel sorting and searching
- Parallel reduce operations
- Work-stealing schedulers

## Usage Examples

### Async/Await

```cpp
use concurrency::async::{spawn, join};

async fn fetch_data(url: str) -> Result<String> {
    // Simulate async I/O
    sleep(100).await;
    Ok("data".to_string())
}

fn main() {
    let future1 = fetch_data("url1");
    let future2 = fetch_data("url2");

    // Run concurrently
    let (result1, result2) = join!(future1, future2);
}
```

### Thread Management

```cpp
use concurrency::threads::Thread;

fn worker(id: i32) {
    println("Worker {} starting", id);
    // Do work...
}

fn main() {
    let mut handles = vec![];

    for i in 0..4 {
        let handle = Thread::spawn(move || worker(i));
        handles.push(handle);
    }

    // Wait for all threads to complete
    for handle in handles {
        handle.join();
    }
}
```

### Channels

```cpp
use concurrency::channels::mpsc::{channel, Receiver, Sender};

fn producer(tx: Sender<i32>) {
    for i in 0..10 {
        tx.send(i);
    }
}

fn consumer(rx: Receiver<i32>) {
    while let Ok(value) = rx.recv() {
        println("Received: {}", value);
    }
}

fn main() {
    let (tx, rx) = channel();

    Thread::spawn(move || producer(tx));
    consumer(rx);
}
```

### Parallel Processing

```cpp
use concurrency::parallel::par_iter;

fn main() {
    let data = vec![1, 2, 3, 4, 5, 6, 7, 8];

    // Parallel processing
    let sum: i32 = data.par_iter()
        .map(|x| x * x)  // Square each number
        .sum();         // Sum results

    println("Sum of squares: {}", sum);
}
```

## Performance Characteristics

- **Work-stealing scheduler**: Efficient load balancing across CPU cores
- **Lock-free algorithms**: High-throughput concurrent data structures
- **Zero-cost futures**: Async code compiles to efficient state machines
- **Memory-efficient channels**: Minimal overhead for message passing

## Integration with Other Modules

- **Memory**: `Arc<T>` from memory module for thread-safe reference counting
- **System**: Integration with OS threading APIs
- **AI**: Parallel processing for machine learning workloads
- **Net**: Async I/O for network operations

## Safety

- Race condition detection in debug builds
- Deadlock prevention hints
- Thread sanitizer integration
- Memory safety across thread boundaries

## Testing

Run the concurrency module tests:

```bash
nova test stdlib/concurrency/
```

Includes tests for:

- Race condition scenarios
- Deadlock detection
- Performance benchmarks
- Integration tests with other modules

## Contributing

When adding concurrency features:

1. Ensure thread safety and race condition freedom
2. Provide both blocking and non-blocking APIs
3. Include comprehensive stress tests
4. Document threading models and ownership semantics
