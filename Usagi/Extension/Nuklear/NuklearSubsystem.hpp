﻿#pragma once

#include <memory>

#include <Usagi/Graphics/Game/OverlayRenderingSubsystem.hpp>
#include <Usagi/Runtime/Input/Keyboard/KeyEventListener.hpp>
#include <Usagi/Runtime/Input/Mouse/MouseEventListener.hpp>
#include <Usagi/Runtime/Window/WindowEventListener.hpp>
#include <Usagi/Game/CollectionSubsystem.hpp>

#include "Nuklear.hpp"
#include "NuklearComponent.hpp"

struct NuklearContext;

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

class NuklearSubsystem
    : public OverlayRenderingSubsystem
    , public CollectionSubsystem<NuklearComponent>
    , public KeyEventListener
    , public MouseEventListener
    , public WindowEventListener
{
    Game *mGame = nullptr;
    std::shared_ptr<Window> mWindow;
    std::shared_ptr<Keyboard> mKeyboard;
    std::shared_ptr<Mouse> mMouse;

    mutable nk_context mContext;
    nk_font_atlas mAtlas;
    mutable nk_buffer mCommandList;
    nk_draw_null_texture mNullTexture;

    static void clipboardPaste(nk_handle usr, struct nk_text_edit *edit);
    static void clipboardCopy(nk_handle usr, const char *text, int len);

    void processElements(const Clock &clock);

	std::shared_ptr<GraphicsPipeline> mPipeline;
	std::shared_ptr<GpuCommandPool> mCommandPool;
    std::shared_ptr<GpuBuffer> mVertexBuffer;
	std::shared_ptr<GpuBuffer> mIndexBuffer;
    std::shared_ptr<GpuImage> mFontTexture;
    std::shared_ptr<GpuImageView> mFontTextureView;
    std::shared_ptr<GpuSampler> mSampler;

    void setup();

public:
    NuklearSubsystem(
        Game *game,
        std::shared_ptr<Window> window,
        std::shared_ptr<Keyboard> keyboard,
        std::shared_ptr<Mouse> mouse);
    ~NuklearSubsystem() override;

    void onKeyStateChange(const KeyEvent &e) override;
    void onMouseMove(const MousePositionEvent &e) override;
    void onMouseButtonStateChange(const MouseButtonEvent &e) override;
    void onMouseWheelScroll(const MouseWheelEvent &e) override;
    void onWindowCharInput(const WindowCharEvent &e) override;

    void update(const Clock &clock) override;

    void createRenderTarget(RenderTargetDescriptor &descriptor) override;
    void createPipelines() override;
    std::shared_ptr<GraphicsCommandList> render(const Clock &clock) override;
};
}
