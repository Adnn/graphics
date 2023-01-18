#pragma once

#include <imgui.h>

#include <graphics/ApplicationGlfw.h>
#include <mutex>


namespace ad {
namespace imguiui {

class ImguiUi
{
public:
    ImguiUi(const graphics::ApplicationGlfw & aApplication);

    ~ImguiUi();

    void renderBackend();

    void newFrame();
    void render();

    bool isCapturingKeyboard();
    bool isCapturingMouse();

    std::mutex mFrameMutex;

private:
    // Could be pimpled
    ImGuiIO & mIo;
};


} // namespace gltfviewer
} // namespace ad
