
#include <iostream>
#include <regex>
#include <string>
#include <sstream>

# define protozero_assert(x)
#include <protozero/pbf_reader.hpp>

static const size_t max_string_length = 60;

static const std::regex printable{"^[A-Za-z0-9]*$"};

std::string decode(const char* data, std::size_t len, const std::string& indent) {
    std::stringstream stream;

    protozero::pbf_reader item(data, len);
    while (item.next()) {
        stream << indent << item.tag() << ": ";
        switch (item.wire_type()) {
            case protozero::pbf_wire_type::varint: {
                // This is int32, int64, uint32, uint64, sint32, sint64, bool, or enum.
                // Try decoding as int64.
                stream << item.get_int64() << "\n";
                break;
            }
            case protozero::pbf_wire_type::fixed64:
                // This is fixed64, sfixed64, or double.
                // Try decoding as a double, because int64_t or uint64_t
                // would probably be encoded as varint.
                stream << item.get_double() << "\n";
                break;
            case protozero::pbf_wire_type::length_delimited: {
                // This is string, bytes, embedded messages, or packed repeated fields.
                auto d = item.get_data();
                try {
                    // Try decoding as a nested message first.
                    auto nested = decode(d.first, d.second, indent + "  ");
                    stream << "\n" << nested;
                } catch (std::exception const&) {
                    std::string str(d.first, d.second);
                    std::smatch match;
                    if (std::regex_match(str, match, printable)) {
                        // Try decoding as a string.
                        if (str.size() > max_string_length) {
                            stream << "\"" << str.substr(0, max_string_length) << "...\n";
                        } else {
                            stream << "\"" << str << "\"\n";
                        }
                    } else {
                        // Fall back.
                        stream << "BINARY DATA\n";
                    }
                }
                break;
            }
            case protozero::pbf_wire_type::fixed32:
                // This is fixed32, sfixed32, or float.
                // Try decoding as a float, because int32_t or uint32_t
                // would probably be encoded as varint.
                stream << item.get_float() << "\n";
                break;
            default:
                throw protozero::unknown_pbf_wire_type_exception();
        }
    }

    return stream.str();
}

int main() {
    try {
        std::string buffer(std::istreambuf_iterator<char>(std::cin.rdbuf()),
                           std::istreambuf_iterator<char>());
        std::cout << decode(buffer.data(), buffer.size(), "");
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        return 1;
    }
    return 0;
}

