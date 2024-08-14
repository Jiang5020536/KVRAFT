#include "util.h"
#include <iostream>

int main() {
    // 测试 DeferClass
    {
        DEFER {
            std::cout << "Scope exited, DeferClass executed!" << std::endl;
        };
    }

    // 测试 DPrintf
    DPrintf("Debug Message: %d, %s", 42, "Hello World");

    // 测试 myAssert
    myAssert(1 + 1 == 2, "Assertion passed.");
    // myAssert(1 + 1 == 3, "Assertion failed!"); // Uncomment to test failure

    // 测试 format
    std::string formattedString = format("Formatted String: %d, %s", 123, "abc");
    std::cout << formattedString << std::endl;

    // 测试 getRandomizedElectionTimeout
    auto timeout = getRandomizedElectionTimeout();
    std::cout << "Randomized election timeout: " << timeout.count() << " ms" << std::endl;

    // 测试 isReleasePort 和 getReleasePort
    short port = 8000;
    if (getReleasePort(port)) {
        std::cout << "Found available port: " << port << std::endl;
    } else {
        std::cout << "Failed to find available port." << std::endl;
    }

    return 0;
}