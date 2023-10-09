/*
* Copyright 2017 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#ifndef AcrylicEffect_DEFINED
#define AcrylicEffect_DEFINED

#include "include/core/SkScalar.h"
#include "include/core/SkTypes.h"
#include "include/core/SkM44.h"
#include "tools/sk_app/Application.h"
#include "tools/sk_app/Window.h"
#include "tools/skui/ModifierKey.h"
#include "include/effects/SkRuntimeEffect.h"

class SkSurface;

class AcrylicEffect : public sk_app::Application, sk_app::Window::Layer {
public:
    AcrylicEffect(int argc, char** argv, void* platformData);
    ~AcrylicEffect() override;

    void onIdle() override;

    void onBackendCreated() override;
    void onPaint(SkSurface*) override;
    bool onChar(SkUnichar c, skui::ModifierKey modifiers) override;
    bool onMouse(int x, int y, skui::InputState state, skui::ModifierKey modifiers) override;

private:
    void updateTitle();

    sk_app::Window* fWindow;
    sk_app::Window::BackendType fBackendType;

    SkV2 mousePos;
};

#endif
