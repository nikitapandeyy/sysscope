#include "cli.hpp"
#include "display.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    minimon::CliConfig config;
    if (!minimon::parse_cli_args(argc, argv, config)) {
        std::cerr << "Error: " << config.error_message << "\n\n";
        minimon::print_help(std::cerr);
        return 1;
    }

    if (config.show_help) {
        minimon::print_help(std::cout);
        return 0;
    }

    if (config.once || config.json) {
        minimon::run_single_shot(config, std::cout);
    } else {
        minimon::run_live_loop(config, std::cout);
    }

    return 0;
}
