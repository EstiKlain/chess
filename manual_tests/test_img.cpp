#include "view/canvas/img.hpp"
#include <iostream>

// PROJECT_ROOT is defined by CMake (see CMakeLists.txt) - it always points
// to the folder where CMakeLists.txt lives, no matter from where you run the .exe
#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

int main() {
    try {
        std::cout << "Testing Img class..." << std::endl;

        std::string path = std::string(PROJECT_ROOT) + "/assets/test.jpg";

        Img img;
        img.read(path, {640, 480}, true);
        img.put_text("Hello, Img!", 50, 400, 1.0, {0, 0, 0});
        img.show();

        std::cout << "Img class test completed successfully!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}