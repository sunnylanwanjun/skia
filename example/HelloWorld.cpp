/*
* Copyright 2017 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#include "example/Samples/HarmonyLightingEffect.h"
#include "example/Samples/AcrylicEffect.h"

using namespace sk_app;

Application* Application::Create(int argc, char** argv, void* platformData) {
    return new AcrylicEffect(argc, argv, platformData);
}