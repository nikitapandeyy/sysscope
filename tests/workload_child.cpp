#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>
#include <thread>
#include <vector>
#include <csignal>
#include <unistd.h>

static volatile sig_atomic_t g_stop = 0;

static void handle_sigterm(int) {
    g_stop = 1;
}

int main(int argc, char* argv[]) {
    // Install SIGTERM handler for clean shutdown
    struct sigaction sa{};
    sa.sa_handler = handle_sigterm;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, nullptr);

    std::string mode = "cpu";
    int duration_sec = 10;
    int mem_mb = 30;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--cpu" && i + 1 < argc) {
            mode = "cpu";
            duration_sec = std::atoi(argv[++i]);
        } else if (arg == "--mem" && i + 1 < argc) {
            mode = "mem";
            mem_mb = std::atoi(argv[++i]);
        } else if (arg == "--duration" && i + 1 < argc) {
            duration_sec = std::atoi(argv[++i]);
        }
    }

    if (mode == "mem") {
        // Allocate memory and touch every page (4096 bytes) so kernel maps it into RSS
        size_t total_bytes = static_cast<size_t>(mem_mb) * 1024 * 1024;
        std::vector<char> buffer(total_bytes);
        std::memset(buffer.data(), 0xAA, total_bytes);

        // Notify parent that allocation is complete and memory is resident
        std::cout << "READY" << std::endl;

        auto start = std::chrono::steady_clock::now();
        while (!g_stop) {
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= duration_sec) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    } else {
        // Mode == cpu: burn CPU cycles in a tight arithmetic loop
        std::cout << "READY" << std::endl;

        auto start = std::chrono::steady_clock::now();
        volatile uint64_t counter = 0;
        while (!g_stop) {
            for (int k = 0; k < 100000; ++k) {
                counter += (k * 3) ^ (k >> 1);
            }
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() >= duration_sec) {
                break;
            }
        }
        (void)counter;
    }

    return 0;
}
