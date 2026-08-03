#pragma once
#include <cstdint>

namespace rf { using Frequency = uint64_t; }

class ReceiverModel {
public:
    enum class Mode { NarrowbandFMAudio, WidebandFMAudio, AMAudio, SpectrumAnalysis };

    void set_target_frequency(rf::Frequency f) {
        printf("[RF] tune %.3f MHz\n", (double)f / 1e6);
    }
    void set_modulation(Mode) {}
    void set_nbfm_configuration(int) {}
    void set_sampling_rate(uint32_t) {}
    void set_baseband_bandwidth(uint32_t) {}
    void set_lna(int32_t) {}
    void set_vga(int32_t) {}
    void set_rf_amp(bool) {}
    void enable()  { printf("[RF] enabled\n"); }
    void disable() { printf("[RF] disabled\n"); }
};
