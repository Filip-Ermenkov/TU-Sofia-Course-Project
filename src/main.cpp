#include <cstdlib>
#include <exception>
#include <iostream>

#include "courier/ConsoleApp.hpp"

int main() {
    // The app handles every logistics error itself. Anything else is a bug or a failure such as
    // running out of memory, so the program stops; the data saved after the last change is safe.
    try {
        courier::ConsoleApp app;
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "Unexpected error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
