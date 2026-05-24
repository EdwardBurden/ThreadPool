# ThreadPool

A lightweight, header-only thread pool for C++20.

## Features
- Automatically sizes itself to available hardware threads.
- Supports any callable with any return type via `EnqueueTask()`.
- Returns `std::future` for optional result retrieval.
- use `WaitForAllTasksComplete()`to sync all tasks.
- Debug logging toggled via `THREADPOOL_DEBUG` compile definition.

## Usage

### Adding to your project
Add `ThreadPool.h` into your project directly.

### Basic usage
```cpp
ThreadPool pool(4);

// Fire and forget
pool.EnqueueTask(myFunction, arg1, arg2);

// With result
auto future = pool.EnqueueTask(myFunction, arg1, arg2);
auto result = future.get(); // blocks until done

// Wait for everything to finish
pool.WaitForAllTasksComplete();
```

### Debug logging
In your CMakeLists.txt:
```cmake
target_compile_definitions(YourTarget PRIVATE THREADPOOL_DEBUG)
```
Or enable automatically for Debug builds:
```cmake
target_compile_definitions(YourTarget PRIVATE
    $<$<CONFIG:Debug>:THREADPOOL_DEBUG>
)
```

## Notes
> ⚠️ This pool uses hard shutdown — tasks still queued when the pool is destroyed
> will be abandoned. Futures for abandoned tasks will throw `std::future_error` 
> on `.get()`. Do not rely on futures outliving the pool.

## Requirements
- C++20
- CMake 3.10+
