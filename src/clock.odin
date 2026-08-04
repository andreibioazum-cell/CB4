package main

monotonic_seconds :: proc() -> f64 {
    now: Clock_Timespec
    if clock_gettime(CLOCK_MONOTONIC, &now) != 0 {
        return 0
    }
    return f64(now.seconds) + f64(now.nanoseconds) / 1_000_000_000.0
}
