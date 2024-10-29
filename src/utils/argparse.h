/*
 *  author: Suhas Vittal
 *  date:   28 October 2024
 * */

#ifndef ARGPARSE_h
#define ARGPARSE_h

#include <initializer_list>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

struct ArgParseResult {
    std::string help_;
    std::unordered_map<std::string_view, std::string> arg_parse_data_;

    ArgParseResult(void) =default;
    ArgParseResult(const ArgParseResult&) =default;

    template <class T>
    inline void operator()(std::string_view arg_name, T& arg_value_ref) {
        auto it = arg_parse_data_.find(arg_name);
        if (it == arg_parse_data_.end()) {
            std::cerr << "Unknown argument \"" << arg_name << "\"\n"
                << help_;
            exit(1);
        }
        std::string value = it->second;
        std::string typenamestr;
        try {
            if constexpr ( std::is_integral<T>::value ) {
                if constexpr (std::is_same<T, bool>::value) {
                    typenamestr = "bool";
                    arg_value_ref = value != "";
                } else if constexpr (std::is_unsigned<T>::value) {
                    typenamestr = "uint64_t";
                    arg_value_ref = static_cast<T>( std::stoull(value) );
                } else {
                    typenamestr = "int64_t";
                    arg_value_ref = static_cast<T>( std::stoll(value) );
                }
            } else if constexpr ( std::is_floating_point<T>::value ) {
                typenamestr = "double";
                arg_value_ref = static_cast<T>( std::stof(value) );
            } else { 
                typenamestr = "std::string";
                // Assume `T = std::string`.
                arg_value_ref = value;
            }
        } catch (...) {
            std::cerr << "Could not parse data for " << arg_name << " as type \"" << typenamestr << "\"\n"
                << help_;
            exit(1);
        }
    }
};

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

/*
 * Parses `argv`. Regarding arguments:
 *  (1) `required`: the first `required.size()` args in `argv` are treated as required arguments. If they
 *      do not exist, `parse` prints usage to stderr and exits with code 1.
 *  (2) `optional`: all mandatory arguments must be provided as a 3-tuple:
 *      (i) flag (-<flag> is used to match the optional argument).
 *      (ii) description
 *      (iii) default value
 *          --> if an optional argument is just a flag (set/unset), then the default value should be the empty
 *              string. It is assumed to be false by default.
 *  Returns `ArgParseResult` (see usage above: arguments can be retrieved 
 *  via `ArgParseResult(<argument>, <variable>)`.
 *      i.e. ArgParseResult pp = parse(...)
 *           pp("foo", bar);
 *      `bar` can be any type.
 * */
ArgParseResult parse(
        int argc,
        char** argv, 
        std::initializer_list<std::string_view> required,
        std::initializer_list< std::tuple<std::string_view, std::string_view, std::string_view> > optional);

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#endif  // ARGPARSE_h
