/*
* Copyright 2017 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#include "example/Samples/AcrylicEffect.h"

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
#include "include/effects/SkImageFilters.h"

#include <string.h>

using namespace sk_app;

AcrylicEffect::AcrylicEffect(int argc, char** argv, void* platformData)
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

AcrylicEffect::~AcrylicEffect() {
    fWindow->detach();
    delete fWindow;
}

void AcrylicEffect::updateTitle() {
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

void AcrylicEffect::onBackendCreated() {
    this->updateTitle();
    fWindow->show();
    fWindow->inval();
}

static std::shared_ptr<SkRuntimeShaderBuilder> GetBlurEffectShaderBuilder() {
    static std::shared_ptr<SkRuntimeShaderBuilder> blurEffectShaderBuilder = nullptr;

    if (blurEffectShaderBuilder) {
        return blurEffectShaderBuilder;
    }

    sk_sp<SkRuntimeEffect> blurEffect_;
    SkString blurString(R"(
        uniform shader content;
        uniform shader blur;

        uniform vec4 rectangle;
        uniform float radius;

        // Simplified version of SDF (signed distance function) for a rounded box
        // from https://www.iquilezles.org/www/articles/distfunctions2d/distfunctions2d.htm
        float roundedRectangleSDF(vec2 position, vec2 box, float radius) {
            vec2 q = abs(position) - box + vec2(radius);
            return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - radius;
        }

        vec4 main(vec2 coord) {
            vec2 shiftRect = (rectangle.zw - rectangle.xy) / 2.0;
            vec2 shiftCoord = coord - rectangle.xy;
            float distanceToClosestEdge = roundedRectangleSDF(
                shiftCoord - shiftRect, shiftRect, radius);

            vec4 c = content.eval(coord);
            if (distanceToClosestEdge > 0.0) {
                // We're outside of the filtered area
                return c;
            }

            vec4 b = blur.eval(coord);
            return b;
        }
    )");

    auto [blurEffect, error] = SkRuntimeEffect::MakeForShader(blurString);
    if (!blurEffect) {
        SkDebugf("GetblurEffectShaderBuilder MakeShader Failed\n");
        return nullptr;
    }
    blurEffect_ = std::move(blurEffect);
    blurEffectShaderBuilder = std::make_shared<SkRuntimeShaderBuilder>(blurEffect_);
    return blurEffectShaderBuilder;
}

void AcrylicEffect::onPaint(SkSurface* surface) {
    auto canvas = surface->getCanvas();
    // Clear background
    canvas->clear(0xFF03080D);

    /////////// purple circle ////////////////
    {
        SkPoint linearPoints[] = {{450, 60}, {290, 190}};
        SkColor linearColors[] = {0xFF7A26D9, 0xFFE444E1};

        SkPaint paint;
        paint.setShader(SkGradientShader::MakeLinear(
                linearPoints, linearColors, nullptr, 2, SkTileMode::kClamp));
        paint.setAntiAlias(true);

        canvas->drawCircle(375, 125, 100, paint);
    }

    {
        SkPaint paint;
        paint.setColor(0xFFEA357C);
        paint.setAntiAlias(true);

        canvas->drawCircle(100, 265, 55, paint);
    }

    {
        SkPoint linearPoints[] = {{180, 125}, {230, 125}};
        SkColor linearColors[] = {0xFFEA334C, 0xFFEC6051};

        SkPaint paint;
        paint.setShader(SkGradientShader::MakeLinear(
                linearPoints, linearColors, nullptr, 2, SkTileMode::kClamp));
        paint.setAntiAlias(true);

        canvas->drawCircle(205, 125, 25, paint);
    }

    SkRect region = SkRect::MakeXYWH(86, 111, 318, 178);
    SkScalar rx = 20;
    SkScalar ry = 20;
    float borderSize = 3.0f;
    auto innerRect = SkRect::MakeXYWH(region.x() + borderSize,
                                      region.y() + borderSize,
                                      region.width() - borderSize * 2.0f,
                                      region.height() - borderSize * 2.0f);

    auto blur = SkImageFilters::Blur(20, 20, SkTileMode::kClamp, nullptr, region);

    /*std::shared_ptr<SkRuntimeShaderBuilder> blurEffectBuilder = GetBlurEffectShaderBuilder();
    float density = 1.0f;
    blurEffectBuilder->uniform("rectangle") =
            SkV4{85.0f * density, 110.0f * density, 405.0f * density, 290.0f * density};
    blurEffectBuilder->uniform("radius") = 20.0f * density;

    std::string_view childShaderNames[] = {"content", "blur"};
    sk_sp<SkImageFilter> inputs[] = {nullptr, blur};

    auto customBlur = SkImageFilters::RuntimeShader(*blurEffectBuilder.get(), childShaderNames, inputs, 2);*/

    SkCanvas::SaveLayerRec offsetScreenRect(&region, nullptr, blur.get(), 0);
    canvas->saveLayer(offsetScreenRect); // new layer with region

    // draw round rect
    {
        canvas->clipRRect(SkRRect::MakeRectXY(region, rx, ry), true);
        canvas->clipRRect(SkRRect::MakeRectXY(innerRect, rx, ry), SkClipOp::kDifference, true);

        SkPoint linearPoints[] = {{region.x(), region.y()}, {region.right(), region.bottom()}};
        SkColor linearColors[] = {0x80FFFFFF, 0x00FFFFFF, 0x00FF48DB, 0x80FF48DB};

        SkPaint paint;
        paint.setShader(SkGradientShader::MakeLinear(
                linearPoints, linearColors, nullptr, 4, SkTileMode::kClamp));
        paint.setAntiAlias(true);
        paint.setStrokeWidth(2);

        canvas->drawRoundRect(region, rx, ry, paint);
    }

    canvas->restore();
}

void AcrylicEffect::onIdle() {
    // Just re-paint continuously
    fWindow->inval();
}

bool AcrylicEffect::onChar(SkUnichar c, skui::ModifierKey modifiers) {
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

bool AcrylicEffect::onMouse(int x, int y, skui::InputState state, skui::ModifierKey modifiers) {
    mousePos = {float(x), float(y)};

    switch (state) {
        case skui::InputState::kUp: {
            // SkDebugf("AcrylicEffect::onMouse Up x=%d, y=%d\n", x, y);
            break;
        }
        case skui::InputState::kDown: {
            // SkDebugf("AcrylicEffect::onMouse Down x=%d, y=%d\n", x, y);
            break;
        }
        case skui::InputState::kMove: {
            // SkDebugf("AcrylicEffect::onMouse Move x=%d, y=%d\n", x, y);
            break;
        }
        default: {
            SkASSERT(false);  // shouldn't see kRight or kLeft here
            break;
        }
    }
    
    return true;
}