#include "core/TextRenderer.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace {

void fail(const std::string& message) {
    std::cerr << message << '\n';
    std::exit(1);
}

void expectEqual(int actual, int expected, const std::string& label) {
    if (actual != expected) {
        std::ostringstream oss;
        oss << label << ": expected " << expected << ", got " << actual;
        fail(oss.str());
    }
}

void testDesktopScaleMapping() {
    expectEqual(TextRenderer::fontSizeForScaleForTests(0.44f), 0, "small scale threshold");
    expectEqual(TextRenderer::fontSizeForScaleForTests(0.45f), 1, "medium lower bound");
    expectEqual(TextRenderer::fontSizeForScaleForTests(0.62f), 1, "medium upper bound");
    expectEqual(TextRenderer::fontSizeForScaleForTests(0.63f), 2, "large scale threshold");
}

} // namespace

int main() {
    testDesktopScaleMapping();
    return 0;
}
