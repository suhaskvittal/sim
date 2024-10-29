/*
 *  author: Suhas Vittal
 *  date:   28 October 2024
 * */

#include "utils/argparse.h"

#include <iomanip>
#include <sstream>

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

using OptionalArgument = std::tuple<std::string_view, std::string_view, std::string_view>;

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#define PRINT_HELP_AND_DIE  std::cerr << out.help_; exit(1)

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

ArgParseResult
parse(int argc,
        char** argv, 
        std::initializer_list<std::string_view> required,
        std::initializer_list<OptionalArgument> optional)
{
    // For simplicity, decrement argc:
    --argc;

    ArgParseResult out;
    // Create help string.
    std::stringstream ss;
    ss << "usage:";
    for (const std::string_view& s : required) {
        ss << " <" << s << ">";
    }
    ss << " [ options ]\n"
       << "------------------------------------------------- Available Options -------------------------------------------------\n";
    for (const auto& [ flag, desc, default_value ] : optional) {
        std::string descs = "\"" + std::string(desc) + "\"";
        std::string defs = default_value.empty() ? "" : "default: " + std::string(default_value);
        ss << "\t-" << std::setw(12) << std::left << flag
            << std::setw(48) << std::left << descs
            << std::setw(16) << std::left << defs << "\n";
    }
    out.help_ = ss.str();
    // Get required arguments 
    if (argc < required.size()) {
        PRINT_HELP_AND_DIE;
    }

    int ii = 1;
    for (const auto& arg : required) {
        out.arg_parse_data_[arg] = std::string(argv[ii]);
        ++ii;
    } 
    // Setup optional argument defaults. 
    for (const auto& [ flag, desc, default_value ] : optional) out.arg_parse_data_[flag] = default_value;
    // Parse optional arguments.
    while (ii <= argc) {
        // Check that this is indeed an option.
        if (argv[ii][0] != '-') {
            std::cerr << "Expected option but got \"" << argv[ii] << "\".\n";
            PRINT_HELP_AND_DIE;
        }
        std::string opt(argv[ii]+1);
        ++ii;
        // Now get value.
        if (out.arg_parse_data_[opt].empty()) {
            // Then this is just a flag.
            out.arg_parse_data_[opt] = "y";
        } else {
            if (ii > argc) {
                std::cerr << "Expected value for argument \"-" << opt << "\" but reached end.\n";
                PRINT_HELP_AND_DIE;
            }
            if (argv[ii][0] == '-') {
                std::cerr << "Expected value for argument \"-" << opt 
                    << "\" but got new argument \"" << argv[ii] << "\".\n";
                PRINT_HELP_AND_DIE;
            }
            out.arg_parse_data_[opt] = argv[ii];
            ++ii;
        }
    }
    return out;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
