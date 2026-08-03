/*
 * loader_sim: host-side replica of Mayhem v2.4.0
 * ExternalItemsMenuLoader::run_external_app() — the function that decides
 * "can't be read". Feed it any .ppma and it replays the exact copy loops,
 * block sizes, and checksum, and reports every memory write range so
 * overlaps/overflows of the 40KB local_sram_1 are visible.
 *
 * Usage: loader_sim file.ppma [file2.ppma ...]
 *
 * Build (standalone): g++ -std=c++17 loader_sim.cpp -o loader_sim
 */
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// ── Firmware constants (v2.4.0) ─────────────────────────────────────────────
static constexpr size_t max_file_block_size = 512;               // file.hpp
static constexpr uint32_t EXT_APP_EXPECTED_CHECKSUM = 0x00000000;
static constexpr uint32_t M4_CODE_BASE = 0x10080000;             // memory_map.hpp
static constexpr uint32_t LOCAL_SRAM_END = 0x10080000 + 40 * 1024;
static constexpr uint32_t M4_CODE_END = M4_CODE_BASE + 32 * 1024;

// application_information_t layout (fields we need; total 84 bytes on device).
// Offsets: 0 memory_location, 4 externalAppEntry, 8 header_version,
// 12 app_version, 16 app_name[16], 32 bitmap_data[32], 64 icon_color,
// 68 menu_location, 72 desired_menu_position(4)... m4_app_tag at 76, m4_app_offset at 80.
struct HeaderView {
    uint32_t memory_location;
    uint32_t external_app_entry;
    uint32_t header_version;
    uint32_t app_version;
    char app_name[17];
    char m4_app_tag[5];
    uint32_t m4_app_offset;
};

static HeaderView parse_header(const uint8_t* h) {
    HeaderView v{};
    memcpy(&v.memory_location, h + 0, 4);
    memcpy(&v.external_app_entry, h + 4, 4);
    memcpy(&v.header_version, h + 8, 4);
    memcpy(&v.app_version, h + 12, 4);
    memcpy(v.app_name, h + 16, 16);
    v.app_name[16] = '\0';
    memcpy(v.m4_app_tag, h + 76, 4);
    v.m4_app_tag[4] = '\0';
    memcpy(&v.m4_app_offset, h + 80, 4);
    return v;
}

// utility.cpp simple_checksum — verbatim logic.
static uint32_t simple_checksum(const uint8_t* ptr, uint32_t length) {
    if (!ptr || length == 0) return 0;
    uint32_t checksum = 0;
    uint32_t i = 0;
    for (; i + 3 < length; i += 4) {
        uint32_t chunk;
        memcpy(&chunk, ptr + i, 4);
        checksum += chunk;
    }
    if (i < length) {
        uint32_t remainder = 0;
        memcpy(&remainder, ptr + i, length - i);
        checksum += remainder;
    }
    return checksum;
}

// Simulated file: sequential reads like the FatFs-backed File class.
struct SimFile {
    std::vector<uint8_t> bytes;
    size_t pos{0};
    // Returns bytes actually read (short at EOF), like File::read.
    size_t read(void* dst, size_t n) {
        size_t avail = bytes.size() - pos;
        size_t take = n < avail ? n : avail;
        memcpy(dst, bytes.data() + pos, take);
        pos += take;
        return take;
    }
    void seek(size_t p) { pos = p; }
};

// Simulated 40KB local_sram_1 with device addressing.
struct SimRam {
    uint8_t mem[40 * 1024];
    bool oob{false};
    uint32_t oob_addr{0};

    uint8_t* at(uint32_t device_addr, size_t len) {
        if (device_addr < M4_CODE_BASE || device_addr + len > LOCAL_SRAM_END) {
            if (!oob) { oob = true; oob_addr = device_addr; }
            // Give the copy somewhere to land so the sim can continue.
            static uint8_t scratch[64 * 1024];
            return scratch;
        }
        return mem + (device_addr - M4_CODE_BASE);
    }
};

static bool run_external_app_sim(const std::string& path) {
    printf("=== %s ===\n", path.c_str());

    SimFile app;
    {
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) { printf("  open failed\n\n"); return false; }
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        rewind(f);
        app.bytes.resize((size_t)sz);
        fread(app.bytes.data(), 1, (size_t)sz, f);
        fclose(f);
    }
    if (app.bytes.size() < 84) { printf("  file smaller than header\n\n"); return false; }

    HeaderView hdr = parse_header(app.bytes.data());
    printf("  size=%zu  name=\"%s\"  tag=%s  m4_off=%u\n",
           app.bytes.size(), hdr.app_name, hdr.m4_app_tag, hdr.m4_app_offset);
    printf("  memory_location=0x%08X  entry=0x%08X  ver=0x%08x\n",
           hdr.memory_location, hdr.external_app_entry, hdr.app_version);

    SimRam ram;
    uint32_t checksum = 0;
    app.seek(0);

    if (hdr.m4_app_offset != 0) {
        // ── copy application image (exact loop from run_external_app) ──
        size_t readValue = 0;
        for (size_t i = 0; i < hdr.m4_app_offset; i += max_file_block_size) {
            size_t bytes_to_read = max_file_block_size;
            if (i + max_file_block_size > hdr.m4_app_offset)
                bytes_to_read = hdr.m4_app_offset - i;
            if (bytes_to_read == 0) break;

            uint8_t* dst = ram.at(hdr.memory_location + (uint32_t)i, bytes_to_read);
            readValue = app.read(dst, bytes_to_read);
            if (readValue == 0) { printf("  READ FAIL in app loop @%zu\n\n", i); return false; }
            checksum += simple_checksum(dst, (uint32_t)readValue);
            if (readValue < max_file_block_size) break;
        }
        printf("  app image:  file[0..%u) -> 0x%08X..0x%08X\n",
               hdr.m4_app_offset, hdr.memory_location, hdr.memory_location + hdr.m4_app_offset);

        // ── copy baseband image (exact loop) ──
        size_t first = hdr.m4_app_offset, last_end = hdr.m4_app_offset;
        for (size_t i = hdr.m4_app_offset;; i += readValue) {
            size_t bytes_to_read = max_file_block_size;
            if ((i % max_file_block_size) != 0)
                bytes_to_read = max_file_block_size - (i % max_file_block_size);
            if (bytes_to_read == 0) break;

            uint32_t target = M4_CODE_BASE + (uint32_t)(i - hdr.m4_app_offset);
            uint8_t* dst = ram.at(target, bytes_to_read);
            readValue = app.read(dst, bytes_to_read);
            // Real File::read returns Ok(0) at EOF -> readResult is truthy, value 0.
            checksum += simple_checksum(dst, (uint32_t)readValue);
            last_end = i + readValue;
            if (readValue != bytes_to_read) break;
        }
        printf("  m4 image:   file[%zu..%zu) -> 0x%08X..0x%08X  (%zu bytes)\n",
               first, last_end, M4_CODE_BASE, M4_CODE_BASE + (uint32_t)(last_end - first),
               last_end - first);

        // Diagnostics: overlaps & bounds
        uint32_t m4_end = M4_CODE_BASE + (uint32_t)(last_end - first);
        if (m4_end > hdr.memory_location)
            printf("  NOTE: m4 copy end 0x%08X overlaps app image start 0x%08X by %u bytes\n",
                   m4_end, hdr.memory_location, m4_end - hdr.memory_location);
        if (m4_end > M4_CODE_END)
            printf("  WARN: m4 image exceeds 32KB m4_code region end 0x%08X\n", M4_CODE_END);
        if (hdr.memory_location + hdr.m4_app_offset > LOCAL_SRAM_END)
            printf("  WARN: app image exceeds local_sram_1 end 0x%08X\n", LOCAL_SRAM_END);
    } else {
        size_t readValue = 0;
        for (size_t i = 0; i < 80 * max_file_block_size; i += max_file_block_size) {
            uint8_t* dst = ram.at(hdr.memory_location + (uint32_t)i, max_file_block_size);
            readValue = app.read(dst, max_file_block_size);
            checksum += simple_checksum(dst, (uint32_t)readValue);
            if (readValue < max_file_block_size) break;
        }
    }

    if (ram.oob)
        printf("  WARN: write outside local_sram_1 detected, first at 0x%08X\n", ram.oob_addr);

    printf("  checksum = 0x%08X  ->  %s\n\n", checksum,
           checksum == EXT_APP_EXPECTED_CHECKSUM ? "LOAD OK (app would launch)"
                                                 : "FAIL (\"can't be read\" modal)");
    return checksum == EXT_APP_EXPECTED_CHECKSUM;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("usage: %s file.ppma [more.ppma ...]\n", argv[0]);
        return 2;
    }
    int failures = 0;
    for (int i = 1; i < argc; ++i)
        if (!run_external_app_sim(argv[i])) ++failures;
    return failures ? 1 : 0;
}
