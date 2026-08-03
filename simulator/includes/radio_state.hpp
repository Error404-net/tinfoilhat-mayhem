#pragma once
// RxRadioState is RAII in Mayhem — enables radio on construct, disables on destroy.
// In the sim it's a no-op.
struct RxRadioState {
    RxRadioState() {}
    ~RxRadioState() {}
};
