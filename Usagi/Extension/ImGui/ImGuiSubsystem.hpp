﻿#pragma once

#include <memory>

#include <Usagi/Graphics/Game/OverlayRenderingSubsystem.hpp>
#include <Usagi/Runtime/Input/Keyboard/KeyEventListener.hpp>
#include <Usagi/Runtime/Input/Mouse/MouseEventListener.hpp>
#include <Usagi/Runtime/Window/WindowEventListener.hpp>
#include <Usagi/Game/CollectionSubsystem.hpp>

#include "ImGuiComponent.hpp"

struct ImGuiContext;

namespace usagi
{
class GpuSampler;
class GpuImageView;
class GpuImage;
struct RenderPassCreateInfo;
class RenderPass;
class Window;
class Game;
class GpuCommandPool;
class GpuBuffer;
class GraphicsPipeline;
class Mouse;

class ImGuiSubsystem
    : public OverlayRenderingSubsystem
    , public CollectionSubsystem<ImGuiComponent>
    , public KeyEventListener
    , public MouseEventListener
    , public WindowEventListener
{
    Game *mGame = nullptr;
    std::shared_ptr<Window> mWindow;
    std::shared_ptr<Keyboard> mKeyboard;
    std::shared_ptr<Mouse> mMouse;

    ImGuiContext *mContext = nullptr;
    bool mMouseJustPressed[5] = { };

    void setupInput();

    void newFrame(float dt);
    void processElements(const Clock &clock);

    void updateMouse();

	std::shared_ptr<GraphicsPipeline> mPipeline;
	std::shared_ptr<GpuCommandPool> mCommandPool;
    std::shared_ptr<RenderPass> mRenderPass;
    std::shared_ptr<GpuBuffer> mVertexBuffer;
	std::shared_ptr<GpuBuffer> mIndexBuffer;
    std::shared_ptr<GpuImage> mFontTexture;
    std::shared_ptr<GpuImageView> mFontTextureView;
    std::shared_ptr<GpuSampler> mSampler;

    void setup();

public:
    ImGuiSubsystem(
        Game *game,
        std::shared_ptr<Window> window,
        std::shared_ptr<Keyboard> keyboard,
        std::shared_ptr<Mouse> mouse);
    ~ImGuiSubsystem() override;

    void createRenderTarget(RenderTargetDescriptor &descriptor) override;
    void createPipelines() override;
    std::shared_ptr<GraphicsCommandList> render(const Clock &clock) override;

    void onKeyStateChange(const KeyEvent &e) override;
    void onMouseButtonStateChange(const MouseButtonEvent &e) override;
    void onMouseWheelScroll(const MouseWheelEvent &e) override;
    void onWindowCharInput(const WindowCharEvent &e) override;

    void update(const Clock &clock) override;
};
}
