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
#include "include/effects/SkPerlinNoiseShader.h"

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
        uniform shader noise;

        uniform vec4 rectangle;
        uniform float radius;

        // Simplified version of SDF (signed distance function) for a rounded box
        // from https://www.iquilezles.org/www/articles/distfunctions2d/distfunctions2d.htm
        float roundedRectangleSDF(vec2 position, vec2 box, float radius) {
            // vec2 q = abs(position) - box + vec2(radius);
            // return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - radius;

            vec2 q = abs(position) - box + vec2(radius);
            float lenQ = length(q);

            // inner rect
            if (q.x <= 0 && q.y <= 0) {
                return -1.0;
            }

            // out rect
            if (q.x > radius || q.y > radius) {
                
                // out round corner
                if (q.x > 0 && q.y > 0) {
                    // return 1.0;
                    return lenQ - radius;
                }
                // out four edge direction
                return max(q.x, q.y) - radius;
                // return 3.0;
            }
            
            // out round rect (clip small corner)
            if (q.x > 0.0 && q.x < radius && q.y > 0.0 && q.y < radius && lenQ > radius) {
                // return 2.0;
                return lenQ - radius;
            }

            // + radius inner rect without corner
            return -2.0;
        }

        vec4 main(vec2 coord) {
            vec2 shiftRect = (rectangle.zw - rectangle.xy) / 2.0;
            vec2 shiftCoord = coord - rectangle.xy;
            float distanceToClosestEdge = roundedRectangleSDF(
                shiftCoord - shiftRect, shiftRect, radius);

            vec4 c = content.eval(coord);
            if (distanceToClosestEdge > 0.0) {
                // We're outside of the filtered area
                
                float dropShadowSize = 20.0;
                if (distanceToClosestEdge < dropShadowSize) {
                    // Emulate drop shadow around the filtered area
                    float darkenFactor = (dropShadowSize - distanceToClosestEdge) / dropShadowSize;
                    // Use exponential drop shadow decay for more pleasant visuals
                    darkenFactor = pow(darkenFactor, 3.0) * 0.3;
                    // Shift towards black, by 10% around the edge, dissipating to 0% further away
                    // return vec4(c.rgb * vec3(0.9 + (1.0 - darkenFactor) / 10.0), c.a);
                    return vec4(vec3(clamp((1.0 - darkenFactor), 0, 1.0)) * c.rgb, c.a);
                }
                
                /*if ( distanceToClosestEdge > 2.0) {
                    return vec4(1.0, 0.0, 0.0, 1.0);    
                } else if ( distanceToClosestEdge > 1.0) {
                    return vec4(1.0, 1.0, 0.0, 1.0);    
                } else {
                    return vec4(0.0, 1.0, 1.0, 1.0);    
                }*/
                return c;
            }

            /*if (distanceToClosestEdge < -1.0) {
                return vec4(0.0, 0.0, 1.0, 1.0);
            } else {
                return vec4(0.0, 1.0, 0.0, 1.0);
            }*/

            vec4 b = blur.eval(coord);
            vec4 n = noise.eval(coord);
            float noiseLuminance = dot(n.rgb, vec3(0.2126, 0.7152, 0.0722));

            // How far are we from the top-left corner?
            float lightenFactor = min(1.0, length(coord - rectangle.xy) / (0.85 * length(rectangle.zw - rectangle.xy)));
            // Shift towards white, by 35% in top left corner, down to 10% in bottom right corner
            lightenFactor = min(1.0, lightenFactor + noiseLuminance);
            return b + (vec4(1.0) - b) * (0.35 - 0.25 * lightenFactor);
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

enum class Type {
    kFractalNoise,
    kTurbulence,
};

sk_sp<SkShader> noise_shader(Type type,
                             float baseFrequencyX,
                             float baseFrequencyY,
                             int numOctaves,
                             float seed,
                             bool stitchTiles,
                             SkISize size) {
    return (type == Type::kFractalNoise)
                   ? SkPerlinNoiseShader::MakeFractalNoise(baseFrequencyX,
                                                           baseFrequencyY,
                                                           numOctaves,
                                                           seed,
                                                           stitchTiles ? &size : nullptr)
                   : SkPerlinNoiseShader::MakeTurbulence(baseFrequencyX,
                                                         baseFrequencyY,
                                                         numOctaves,
                                                         seed,
                                                         stitchTiles ? &size : nullptr);
}

void DrawText(SkCanvas* canvas, SkScalar fontSize, std::string content, SkScalar x, SkScalar y) {
    //// Draw a message with a nice black paint
    SkFont font;
    font.setSubpixel(true);
    font.setSize(fontSize);
    font.setEmbolden(true);

    SkPaint paint;
    paint.setColor(0x80FFFFFF);
    paint.setStyle(SkPaint::Style::kFill_Style);

    // Draw the text
    canvas->drawSimpleText(content.c_str(),
                           content.length(),
                           SkTextEncoding::kUTF8,
                           x,
                           y,
                           font,
                           paint);
}

void AcrylicEffect::onPaint(SkSurface* surface) {
    auto canvas = surface->getCanvas();
    canvas->save();
    // Clear background
    canvas->clear(0xFF03080D);
    canvas->scale(1.5, 1.5);

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

    SkRect blurRect = SkRect::MakeXYWH(86, 111, 318, 178);
    SkScalar radius = 20;
    float borderSize = 4.0f;
    
    float darkenSize = 10.0f;
    // 包含黑边的最大范围
    auto wholeRect = SkRect::MakeXYWH(blurRect.x() - darkenSize,
                                    blurRect.y() - darkenSize,
                                    blurRect.width() + darkenSize * 2.0f,
                                    blurRect.height() + darkenSize * 2.0f);
    
    // 模糊shader
    auto blur = SkImageFilters::Blur(20, 20, SkTileMode::kClamp, nullptr, wholeRect);

    // 噪声Shader，数值越小，随机越少
    auto noise = SkPerlinNoiseShader::MakeFractalNoise(0.45f, 0.45f, 16, 2.0f);
    auto noiseFilter = SkImageFilters::Shader(noise);

    std::shared_ptr<SkRuntimeShaderBuilder> blurEffectBuilder = GetBlurEffectShaderBuilder();
    if (!blurEffectBuilder) 
    {
        return;
    }
    blurEffectBuilder->uniform("rectangle") =
            SkV4{blurRect.left(), blurRect.top(), blurRect.right(), blurRect.bottom()};
    blurEffectBuilder->uniform("radius") = radius;

    std::string_view childShaderNames[] = {"content", "blur", "noise"};
    sk_sp<SkImageFilter> inputs[] = {nullptr, blur, noiseFilter};

    auto innterBlurOtherOriginFilter = SkImageFilters::RuntimeShader(*blurEffectBuilder.get(), childShaderNames, inputs, 3);

    SkCanvas::SaveLayerRec offsetScreenRect(
            &wholeRect, nullptr, innterBlurOtherOriginFilter.get(), 0);
    canvas->saveLayer(offsetScreenRect); // new layer with region

    // draw edge light
    {
        auto noRimLightRect = SkRect::MakeXYWH(blurRect.x() + borderSize,
                                          blurRect.y() + borderSize,
                                          blurRect.width() - borderSize * 2.0f,
                                          blurRect.height() - borderSize * 2.0f);

        canvas->clipRRect(SkRRect::MakeRectXY(blurRect, radius, radius), true);
        canvas->clipRRect(
                SkRRect::MakeRectXY(noRimLightRect, radius, radius), SkClipOp::kDifference, true);

        SkPoint linearPoints[] = {{blurRect.x(), blurRect.y()}, {blurRect.right(), blurRect.bottom()}};
        SkColor linearColors[] = {0x80FFFFFF, 0x00FFFFFF, 0x00FF48DB, 0x80FF48DB};

        SkPaint paint;
        paint.setShader(SkGradientShader::MakeLinear(
                linearPoints, linearColors, nullptr, 4, SkTileMode::kClamp));
        paint.setAntiAlias(true);
        paint.setStrokeWidth(2);

        canvas->drawRoundRect(blurRect, radius, radius, paint);
    }

    canvas->restore();

    {
        DrawText(canvas, 14, "MEMBERSHIP", 100, 140);
        DrawText(canvas, 18, "JAMES APPLESEED", 100, 240);
        DrawText(canvas, 13, "PUSHING-PIXELS", 100, 265);
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