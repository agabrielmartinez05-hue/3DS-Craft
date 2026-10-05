#include "App.hpp"
#include "Error.hpp"
#include <new>

// libctru otherwise relocates main onto a 32 KiB stack, even with a CIA StackSize.
extern "C" {
u32 __stacksize__ = 256 * 1024;
}

int main() {
    voxel::Renderer renderer;
    try {
        renderer.initialize();
        bool new3ds = false;
        const Result hardware = APT_CheckNew3DS(&new3ds);
        if (R_FAILED(hardware))
            throw voxel::Error("New 3DS detection failed: 0x%08lX",
                               static_cast<unsigned long>(hardware));
        if (!new3ds)
            throw voxel::Error("This application requires a New Nintendo 3DS / New 2DS XL");
        osSetSpeedupEnable(true);
        voxel::App app(renderer);
        app.run();
    } catch (const std::bad_alloc&) {
        renderer.errorScreen("Out of application RAM while allocating game data");
        return 1;
    } catch (const std::exception& error) {
        renderer.errorScreen(error.what());
        return 1;
    } catch (...) {
        renderer.errorScreen("Unknown C++ exception in application");
        return 1;
    }
    return 0;
}
