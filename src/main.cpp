#include "core/Application.h"

int main(int argc, char* argv[]) {
    // Basic setup for cross-platform
    const int SCREEN_WIDTH = 400 + 320; 
    const int SCREEN_HEIGHT = 240;

    Application app("Balatro Demake", SCREEN_WIDTH, SCREEN_HEIGHT);
    app.run();

    return 0;
}
