#include <iostream>
#include "pbf_reader.hpp"

int main() {
    std::string buffer;
    std::ios_base::sync_with_stdio(false);
    try {
        while(std::cin) {
            getline(std::cin, buffer);
            mapbox::util::pbf item(buffer.data(), buffer.size());
            item.next();
            item.get_double();
        }        
    } catch (std::exception const& ex) {
        std::clog << ex.what() << "\n";
        return -1;
    }

    return 0;
}