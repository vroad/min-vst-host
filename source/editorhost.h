//-----------------------------------------------------------------------------
// LICENSE
// (c) 2024, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this
//     software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/vst/ivstcomponent.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "source/media/audioclient.h"
#include "source/platform/iapplication.h"
#include "source/platform/iwindow.h"
#include <optional>
#include <string>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
namespace EditorHost {

class WindowController;

struct PluginConfig {
  std::string plugin_path;
  std::optional<std::string> uid;
  std::optional<std::string> plugin_state_path;
};

//------------------------------------------------------------------------
class App : public IApplication {
public:
  ~App() noexcept override;
  void init(const std::vector<std::string> &cmdArgs) override;
  void terminate() override;

private:
  enum OpenFlags {
    kSetComponentHandler = 1 << 0,
    kSecondWindow = 1 << 1,
  };
  void openEditor(const PluginConfig &cfg, uint32 flags);
  void createViewAndShow(IEditController *controller);
  void startAudioClient();

  VST3::Hosting::Module::Ptr module{nullptr};
  IPtr<PlugProvider> plugProvider{nullptr};
  Vst::HostApplication pluginContext;
  WindowPtr window;
  std::shared_ptr<WindowController> windowController;
  AudioClientPtr audioClient;
  std::string pluginStatePath;
  PluginConfig pluginConfig;
  IPtr<IComponent> plugProviderComponent;
};

//------------------------------------------------------------------------
} // namespace EditorHost
} // namespace Vst
} // namespace Steinberg
