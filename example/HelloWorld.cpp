/*
* Copyright 2017 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#include "example/HelloWorld.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontTypes.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPoint.h"
#include "include/core/SkRect.h"
#include "include/core/SkRRect.h"
#include "include/core/SkShader.h"
#include "include/core/SkString.h"
#include "include/core/SkSurface.h"
#include "include/core/SkTileMode.h"
#include "include/effects/SkGradientShader.h"
#include "tools/sk_app/DisplayParams.h"

#include <string.h>

using namespace sk_app;

struct SkButton {
    SkV4 color = SkV4{1.0f, 1.0f, 1.0f, 1.0f};
    SkRect rect;
    float maxDistance = 100.0f;
    float attenuation = 0.5f;
    SkScalar rx = 0.0f;
    SkScalar ry = 0.0f;
    bool border = false;
    float borderSize = 1.5f;
    float borderMaxDistancee = 100.0f;
    float borderAttenuation = 0.8f;
    std::string text = "";
    int fontSize = 14;

    void draw(SkCanvas* canvas, const SkV2& mousePos) {
        // draw button
        {
            SkPaint paint;

            // Draw a rectangle with red paint

            std::shared_ptr<SkRuntimeShaderBuilder> lightBuilder = getBtnShaderBuilder();
            if (!lightBuilder) {
                return;
            }
            lightBuilder->uniform("btnLTPos") = SkV2{rect.x(), rect.y()};
            lightBuilder->uniform("btnSize") = SkV2{rect.width(), rect.height()};
            lightBuilder->uniform("mousePos") = SkV2{mousePos.x, mousePos.y};
            lightBuilder->uniform("btnColor") = color;
            lightBuilder->uniform("maxDistance") = maxDistance;
            lightBuilder->uniform("attenuation") = attenuation;

            sk_sp<SkShader> shader = lightBuilder->makeShader();
            paint.setShader(shader);

            if (rx > 0.0 || ry > 0.0) {
                canvas->drawRoundRect(rect, rx, ry, paint);
            } else {
                canvas->drawRect(rect, paint);
            }
        }

        // draw border
        if (border) {
            canvas->save();

            SkPaint paint;

            // Draw a rectangle with red paint

            std::shared_ptr<SkRuntimeShaderBuilder> lightBuilder = getBorderShaderBuilder();
            if (!lightBuilder) {
                return;
            }
            lightBuilder->uniform("mousePos") = SkV2{mousePos.x, mousePos.y};
            lightBuilder->uniform("maxDistance") = borderMaxDistancee;
            lightBuilder->uniform("attenuation") = borderAttenuation;

            sk_sp<SkShader> shader = lightBuilder->makeShader();
            paint.setShader(shader);

            if (rx > 0.0 || ry > 0.0) {
                canvas->clipRRect(SkRRect::MakeRectXY(rect, rx, ry), true);
                auto innerRect = SkRect::MakeXYWH(rect.x() + borderSize,
                                                  rect.y() + borderSize,
                                                  rect.width() - borderSize * 2.0f,
                                                  rect.height() - borderSize * 2.0f);
                canvas->clipRRect(
                        SkRRect::MakeRectXY(innerRect, rx, ry), SkClipOp::kDifference, true);

                canvas->drawRoundRect(rect, rx, ry, paint);
            } else {
                canvas->clipRect(rect, true);
                auto innerRect = SkRect::MakeXYWH(rect.x() + borderSize,
                                                  rect.y() + borderSize,
                                                  rect.width() - borderSize * 2.0f,
                                                  rect.height() - borderSize * 2.0f);
                canvas->clipRect(innerRect, SkClipOp::kDifference, true);

                canvas->drawRect(rect, paint);
            }
            canvas->restore();
        }

        if (text.length() > 1)
        {
            //// Draw a message with a nice black paint
            SkFont font;
            font.setSubpixel(true);
            font.setSize(fontSize);

            SkPaint paint;
            paint.setColor(SK_ColorWHITE);

            SkRect fontRect;
            font.measureText(text.c_str(), text.length(),
                             SkTextEncoding::kUTF8,
                             &fontRect);

            // Draw the text
            canvas->drawSimpleText(text.c_str(),
                                   text.length(),
                                   SkTextEncoding::kUTF8,
                                   rect.x() + (rect.width() - fontRect.width()) / 2.0f,
                                   rect.y() + (rect.height() + fontRect.height()) / 2.0f,
                                   font,
                                   paint);
        }
    }

private:
    std::shared_ptr<SkRuntimeShaderBuilder> getBtnShaderBuilder() {
        static std::shared_ptr<SkRuntimeShaderBuilder> btnShaderBuilder = nullptr;

        if (btnShaderBuilder) {
            return btnShaderBuilder;
        }

        sk_sp<SkRuntimeEffect> lightEffect_;
        SkString ligthString(R"(
        uniform vec2 btnLTPos;
        uniform vec2 btnSize;
        uniform vec2 mousePos;
        uniform vec4 btnColor;
        uniform float maxDistance;
        uniform float attenuation;
        float4 main(float2 fragCoord) {
            float len = length(fragCoord - mousePos);
            float strength = saturate(1.0 - len / maxDistance) * attenuation;
            float dx = mousePos.x - btnLTPos.x;
            float dy = mousePos.y - btnLTPos.y;
            if (dx >= 0.0 && dx <= btnSize.x && dy >= 0.0 && dy <= btnSize.y)
            {
                return float4(strength) + btnColor;
            }
            else
            {
                return btnColor;
            }
            //return half4((fragCoord.x - btnLTPos.x) / btnSize.x, (fragCoord.y - btnLTPos.y) / btnSize.y, 0.0, 0.0);
        }
    )");

        auto [lightEffect, error] = SkRuntimeEffect::MakeForShader(ligthString);
        if (!lightEffect) {
            SkDebugf("HelloWorld::getBtnShaderBuilder MakeShader Failed\n");
            return nullptr;
        }
        lightEffect_ = std::move(lightEffect);
        btnShaderBuilder = std::make_shared<SkRuntimeShaderBuilder>(lightEffect_);
        return btnShaderBuilder;
    }

    std::shared_ptr<SkRuntimeShaderBuilder> getBorderShaderBuilder() {
        static std::shared_ptr<SkRuntimeShaderBuilder> borderShaderBuilder = nullptr;

        if (borderShaderBuilder) {
            return borderShaderBuilder;
        }

        sk_sp<SkRuntimeEffect> lightEffect_;
        SkString ligthString(R"(
        uniform vec2 mousePos;
        uniform float maxDistance;
        uniform float attenuation;
        float4 main(float2 fragCoord) {
            float len = length(fragCoord - mousePos);
            float strength = saturate(1.0 - len / maxDistance) * attenuation;
            return float4(strength);
        }
    )");

        auto [lightEffect, error] = SkRuntimeEffect::MakeForShader(ligthString);
        if (!lightEffect) {
            SkDebugf("HelloWorld::getBorderShaderBuilder MakeShader Failed\n");
            return nullptr;
        }
        lightEffect_ = std::move(lightEffect);
        borderShaderBuilder = std::make_shared<SkRuntimeShaderBuilder>(lightEffect_);
        return borderShaderBuilder;
    }
};

Application* Application::Create(int argc, char** argv, void* platformData) {
    return new HelloWorld(argc, argv, platformData);
}

HelloWorld::HelloWorld(int argc, char** argv, void* platformData)
#if defined(SK_GL)
        : fBackendType(Window::kNativeGL_BackendType)
#elif defined(SK_VULKAN)
        : fBackendType(Window::kVulkan_BackendType)
#elif defined(SK_DAWN)
        : fBackendType(Window::kDawn_BackendType)
#else
        : fBackendType(Window::kRaster_BackendType)
#endif
        {
    SkGraphics::Init();

    fWindow = Window::CreateNativeWindow(platformData);
    fWindow->setRequestedDisplayParams(DisplayParams());

    // register callbacks
    fWindow->pushLayer(this);

    fWindow->attach(fBackendType);
}

HelloWorld::~HelloWorld() {
    fWindow->detach();
    delete fWindow;
}

void HelloWorld::updateTitle() {
    if (!fWindow) {
        return;
    }

    SkString title("Hello World ");
    if (Window::kRaster_BackendType == fBackendType) {
        title.append("Raster");
    } else {
#if defined(SK_GL)
        title.append("GL");
#elif defined(SK_VULKAN)
        title.append("Vulkan");
#elif defined(SK_DAWN)
        title.append("Dawn");
#else
        title.append("Unknown GPU backend");
#endif
    }

    fWindow->setTitle(title.c_str());
}

void HelloWorld::onBackendCreated() {
    this->updateTitle();
    fWindow->show();
    fWindow->inval();
}

void HelloWorld::onPaint(SkSurface* surface) {
    auto canvas = surface->getCanvas();
    
    // Clear background
    canvas->clear(SkColorSetARGB(255, 40, 40, 40));

    auto size = canvas->getBaseLayerSize();
    // SkDebugf("HelloWorld::onPaint layer size x=%d, y=%d\n", size.fWidth, size.fHeight);

    int centerX = size.fWidth / 2;
    int centerY = size.fHeight / 2;

    int btnIndex = 1;
    int btnWidth = 114;
    int btnHeight = 49;
    int intervalX = 10;
    int btnCount = 4;
    int beginX = centerX - btnCount / 2.0f * (btnWidth + intervalX);
    int beginY = 73;
    float maxDistance = 500.0f;
    float attenuation = 0.1f;

    //// Draw a message with a nice black paint
    SkFont font;
    font.setSubpixel(true);
    font.setSize(20);

    SkPaint paint;
    paint.setColor(SK_ColorWHITE); 

    // Draw the text
    static const char message[] = "OpenHarmony Lighting Effect";
    canvas->drawSimpleText(
            message, strlen(message), SkTextEncoding::kUTF8, centerX - 134, beginY, font, paint);

    // up banner
    int bannerBegin = 45;
    beginY += 70;

    SkButton bannerBtn;
    bannerBtn.color = SkV4{0.0f, 0.0f, 0.0f, 0.0f};
    bannerBtn.rect = SkRect::MakeXYWH(bannerBegin, beginY, size.fWidth - bannerBegin * 2, btnHeight);
    bannerBtn.maxDistance = maxDistance;
    bannerBtn.attenuation = attenuation;
    bannerBtn.draw(canvas, mousePos);

    // up four small btn
    SkV4 btnColors[] = {SkV4{ 83.0f / 255.0f,  83.0f / 255.0f,  83.0f / 255.0f, 1.0f},
                        SkV4{  0.0f / 255.0f,  93.0f / 255.0f, 185.0f / 255.0f, 1.0f},
                        SkV4{173.0f / 255.0f,   0.0f / 255.0f,  82.0f / 255.0f, 1.0f},
                        SkV4{  0.0f / 255.0f, 148.0f / 255.0f, 126.0f / 255.0f, 1.0f}};

    maxDistance = 100.0f;
    attenuation = 0.3f;

    for (int i = 0; i < btnCount; i++) {
        SkButton upBtn;
        upBtn.color = btnColors[i];
        upBtn.rect = SkRect::MakeXYWH(beginX, beginY, btnWidth, btnHeight);
        upBtn.maxDistance = maxDistance;
        upBtn.attenuation = attenuation;
        upBtn.text = "button " + std::to_string(btnIndex++);
        upBtn.draw(canvas, mousePos);
        
        beginX += btnWidth + intervalX;
    }
    
    // down big banner
    paint.setColor(SkColorSetARGB(255.0f, 51.0f, 51.0f, 51.0f));
    beginY += 77;
    canvas->drawRect(SkRect::MakeXYWH(bannerBegin, beginY, size.fWidth - bannerBegin * 2, 184),
                     paint);

    // middle small btn
    intervalX = 15;
    beginX = centerX - btnCount / 2.0f * (btnWidth + intervalX);
    beginY += 30;
    btnHeight += 5;
    for (int i = 0; i < btnCount; i++) {
        SkButton middleBtn;
        middleBtn.color = btnColors[i];
        middleBtn.rect = SkRect::MakeXYWH(beginX, beginY, btnWidth, btnHeight);
        middleBtn.maxDistance = maxDistance;
        middleBtn.attenuation = attenuation;
        middleBtn.border = true;
        middleBtn.text = "button " + std::to_string(btnIndex++);
        middleBtn.draw(canvas, mousePos);
        beginX += btnWidth + intervalX;
    }

    // down small btn
    btnCount = 3;
    beginX = centerX - btnCount / 2.0f * (btnWidth + intervalX);
    beginY += 70;
    for (int i = 0; i < btnCount; i++) {

        SkButton downBtn;
        downBtn.color = btnColors[i];
        downBtn.rect = SkRect::MakeXYWH(beginX, beginY, btnWidth, btnHeight);
        downBtn.maxDistance = maxDistance;
        downBtn.attenuation = attenuation;
        downBtn.border = true;
        downBtn.rx = 10.0f;
        downBtn.ry = 10.0f;
        downBtn.text = "button " + std::to_string(btnIndex++);
        downBtn.draw(canvas, mousePos);

        beginX += btnWidth + intervalX;
    }
}

void HelloWorld::onIdle() {
    // Just re-paint continuously
    fWindow->inval();
}

bool HelloWorld::onChar(SkUnichar c, skui::ModifierKey modifiers) {
    if (' ' == c) {
        if (Window::kRaster_BackendType == fBackendType) {
#if defined(SK_GL)
            fBackendType = Window::kNativeGL_BackendType;
#elif defined(SK_VULKAN)
            fBackendType = Window::kVulkan_BackendType;
#elif defined(SK_DAWN)
            fBackendType = Window::kDawn_BackendType;
#else
            SkDebugf("No GPU backend configured\n");
            return true;
#endif
        } else {
            fBackendType = Window::kRaster_BackendType;
        }
        fWindow->detach();
        fWindow->attach(fBackendType);
    }
    return true;
}

bool HelloWorld::onMouse(int x, int y, skui::InputState state, skui::ModifierKey modifiers) {
    mousePos = {float(x), float(y)};

    switch (state) {
        case skui::InputState::kUp: {
            // SkDebugf("HelloWorld::onMouse Up x=%d, y=%d\n", x, y);
            break;
        }
        case skui::InputState::kDown: {
            // SkDebugf("HelloWorld::onMouse Down x=%d, y=%d\n", x, y);
            break;
        }
        case skui::InputState::kMove: {
            // SkDebugf("HelloWorld::onMouse Move x=%d, y=%d\n", x, y);
            break;
        }
        default: {
            SkASSERT(false);  // shouldn't see kRight or kLeft here
            break;
        }
    }
    
    return true;
}