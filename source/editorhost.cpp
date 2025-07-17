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

#include "source/editorhost.h"
#include "base/source/fdebug.h"
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "public.sdk/source/vst/vstpresetfile.h"
#include "source/platform/appinit.h"
#include "toml11/toml.hpp"
#include <cassert>
#include <cstdio>
#include <vector>

//------------------------------------------------------------------------
TOML11_DEFINE_CONVERSION_NON_INTRUSIVE(
    ::Steinberg::Vst::EditorHost::PluginConfig, plugin_path, uid,
    plugin_state_path, input_bus_arrangements, output_bus_arrangements);

//------------------------------------------------------------------------
namespace Steinberg {

//------------------------------------------------------------------------
inline bool operator==(const ViewRect &r1, const ViewRect &r2) {
  return memcmp(&r1, &r2, sizeof(ViewRect)) == 0;
}

//------------------------------------------------------------------------
inline bool operator!=(const ViewRect &r1, const ViewRect &r2) {
  return !(r1 == r2);
}

namespace Vst {
namespace EditorHost {

static AppInit gInit(std::make_unique<App>());

//------------------------------------------------------------------------
class WindowController : public IWindowController, public IPlugFrame {
public:
  WindowController(const IPtr<IPlugView> &plugView);
  ~WindowController() noexcept override;

  void onShow(IWindow &w) override;
  void onClose(IWindow &w) override;
  void onResize(IWindow &w, Size newSize) override;
  Size constrainSize(IWindow &w, Size requestedSize) override;
  void onContentScaleFactorChanged(IWindow &window,
                                   float newScaleFactor) override;

  // IPlugFrame
  tresult PLUGIN_API resizeView(IPlugView *view, ViewRect *newSize) override;

  void closePlugView();

private:
  tresult PLUGIN_API queryInterface(const TUID _iid, void **obj) override {
    if (FUnknownPrivate::iidEqual(_iid, FUnknown::iid)) {
      *obj = static_cast<FUnknown *>(this);
      addRef();
      return kResultTrue;
    } else if (FUnknownPrivate::iidEqual(_iid, IPlugFrame::iid)) {
      *obj = static_cast<IPlugFrame *>(this);
      addRef();
      return kResultTrue;
    }
    if (window)
      return window->queryInterface(_iid, obj);
    return kNoInterface;
  }
  // we do not care here of the ref-counting. A plug-in call of release should
  // not destroy this class!
  uint32 PLUGIN_API addRef() override { return 1000; }
  uint32 PLUGIN_API release() override { return 1000; }

  IPtr<IPlugView> plugView;
  IWindow *window{nullptr};
  bool resizeViewRecursionGard{false};
};

//------------------------------------------------------------------------
class ComponentHandler : public IComponentHandler {
public:
  tresult PLUGIN_API beginEdit(ParamID id) override {
    SMTG_DBPRT1("beginEdit called (%d)\n", id);
    return kNotImplemented;
  }
  tresult PLUGIN_API performEdit(ParamID id,
                                 ParamValue valueNormalized) override {
    SMTG_DBPRT2("performEdit called (%d, %f)\n", id, valueNormalized);
    return kNotImplemented;
  }
  tresult PLUGIN_API endEdit(ParamID id) override {
    SMTG_DBPRT1("endEdit called (%d)\n", id);
    return kNotImplemented;
  }
  tresult PLUGIN_API restartComponent(int32 flags) override {
    SMTG_DBPRT1("restartComponent called (%d)\n", flags);
    return kNotImplemented;
  }

private:
  tresult PLUGIN_API queryInterface(const TUID /*_iid*/,
                                    void ** /*obj*/) override {
    return kNoInterface;
  }
  // we do not care here of the ref-counting. A plug-in call of release should
  // not destroy this class!
  uint32 PLUGIN_API addRef() override { return 1000; }
  uint32 PLUGIN_API release() override { return 1000; }
};

static ComponentHandler gComponentHandler;

//------------------------------------------------------------------------
App::~App() noexcept { terminate(); }

//------------------------------------------------------------------------
void App::openEditor(const PluginConfig &cfg, uint32 flags) {
  VST3::Optional<VST3::UID> effectID = VST3::UID::fromString(*cfg.uid);
  if (auto const &s = cfg.plugin_state_path)
    pluginStatePath = *s;

  std::string error;
  module = VST3::Hosting::Module::create(cfg.plugin_path, error);
  if (!module) {
    std::string reason = "Could not create Module for file:";
    reason += cfg.plugin_path;
    reason += "\nError: ";
    reason += error;
    IPlatform::instance().kill(-1, reason);
  }

  auto factory = module->getFactory();
  if (auto factoryHostContext = IPlatform::instance().getPluginFactoryContext())
    factory.setHostContext(factoryHostContext);
  for (auto &classInfo : factory.classInfos()) {
    if (classInfo.category() == kVstAudioEffectClass) {
      if (effectID) {
        if (*effectID != classInfo.ID())
          continue;
      }
      plugProvider = owned(new PlugProvider(factory, classInfo, true));
      if (plugProvider->initialize() == false)
        plugProvider = nullptr;
      else
        plugProviderComponent = owned(plugProvider->getComponent());
      break;
    }
  }
  if (!plugProvider) {
    if (effectID)
      error = "No VST3 Audio Module Class with UID " + effectID->toString() +
              " found in file ";
    else
      error = "No VST3 Audio Module Class found in file ";
    error += cfg.plugin_path;
    IPlatform::instance().kill(-1, error);
  }

  auto editController = plugProvider->getController();
  if (!editController) {
    error = "No EditController found (needed for allowing editor) in file " +
            cfg.plugin_path;
    IPlatform::instance().kill(-1, error);
  }
  editController->release(); // plugProvider does an addRef

  if (flags & kSetComponentHandler) {
    SMTG_DBPRT0("setComponentHandler is used\n");
    editController->setComponentHandler(&gComponentHandler);
  }

  if (!pluginStatePath.empty()) {
    if (IPtr<IBStream> stream =
            owned(FileStream::open(pluginStatePath.c_str(), "r"))) {
      FUID uid;
      if (plugProvider->getComponentUID(uid) == kResultTrue) {
        PresetFile::loadPreset(stream, uid, plugProviderComponent,
                               editController);
      }
    }
  }

  SMTG_DBPRT1("Open Editor for %s...\n", cfg.plugin_path.c_str());
  createViewAndShow(editController);

  if (flags & kSecondWindow) {
    SMTG_DBPRT0("Open 2cd Editor...\n");
    createViewAndShow(editController);
  }
}

//------------------------------------------------------------------------
void App::createViewAndShow(IEditController *controller) {
  auto view = owned(controller->createView(ViewType::kEditor));
  if (!view) {
    IPlatform::instance().kill(
        -1, "EditController does not provide its own editor");
  }

  ViewRect plugViewSize{};
  auto result = view->getSize(&plugViewSize);
  if (result != kResultTrue) {
    IPlatform::instance().kill(-1, "Could not get editor view size");
  }

  auto viewRect = ViewRectToRect(plugViewSize);

  windowController = std::make_shared<WindowController>(view);
  window = IPlatform::instance().createWindow("Editor", viewRect.size,
                                              view->canResize() == kResultTrue,
                                              windowController);
  if (!window) {
    IPlatform::instance().kill(-1, "Could not create window");
  }

  window->show();
}

//------------------------------------------------------------------------
void App::startAudioClient() {
  OPtr<IComponent> component = plugProvider->getComponent();
  OPtr<IEditController> controller = plugProvider->getController();
  auto midiMapping = U::cast<IMidiMapping>(controller);

  std::vector<SpeakerArrangement> inputArrangements;
  if (pluginConfig.input_bus_arrangements) {
    for (const auto &s : *pluginConfig.input_bus_arrangements) {
      SpeakerArrangement arr{};
      if (SpeakerArr::getSpeakerArrangementFromString(s.c_str(), arr))
        inputArrangements.push_back(arr);
    }
  }

  std::vector<SpeakerArrangement> outputArrangements;
  if (pluginConfig.output_bus_arrangements) {
    for (const auto &s : *pluginConfig.output_bus_arrangements) {
      SpeakerArrangement arr{};
      if (SpeakerArr::getSpeakerArrangementFromString(s.c_str(), arr))
        outputArrangements.push_back(arr);
    }
  }

  audioClient =
      AudioClient::create("MinVSTHost Core", component, midiMapping,
                          inputArrangements, outputArrangements);
}

//------------------------------------------------------------------------
void App::init(const std::vector<std::string> &cmdArgs) {
  uint32 flags{};
  for (auto it = cmdArgs.begin(), end = cmdArgs.end(); it != end; ++it) {
    if (*it == "--componentHandler")
      flags |= kSetComponentHandler;
    else if (*it == "--secondWindow")
      flags |= kSecondWindow;
  }

  if (cmdArgs.empty()) {
    auto helpText = R"(
usage: EditorHost [options] configPath

options:

--componentHandler
  set optional component handler on edit controller

--secondWindow
  create a second window
)";

    IPlatform::instance().kill(0, helpText);
  }

  PluginContextFactory::instance().setPluginContext(&pluginContext);

  auto configPath = cmdArgs.back();
  auto config = toml::parse(configPath);
  PluginConfig cfg = toml::get<PluginConfig>(config);
  pluginConfig = cfg;

  openEditor(cfg, flags);

  startAudioClient();
}

//------------------------------------------------------------------------
void App::terminate() {
  audioClient.reset();
  if (windowController)
    windowController->closePlugView();
  windowController.reset();

  if (plugProvider && !pluginStatePath.empty()) {
    if (IPtr<IBStream> stream =
            owned(FileStream::open(pluginStatePath.c_str(), "w"))) {
      FUID uid;
      if (plugProvider->getComponentUID(uid) == kResultTrue) {
        IComponent *component = plugProvider->getComponent();
        IEditController *controller = plugProvider->getController();
        PresetFile::savePreset(stream, uid, component, controller);
      }
    }
  }

  plugProvider.reset();
  module.reset();
  PluginContextFactory::instance().setPluginContext(nullptr);
}

//------------------------------------------------------------------------
WindowController::WindowController(const IPtr<IPlugView> &plugView)
    : plugView(plugView) {}

//------------------------------------------------------------------------
WindowController::~WindowController() noexcept {}

//------------------------------------------------------------------------
void WindowController::onShow(IWindow &w) {
  SMTG_DBPRT1("onShow called (%p)\n", (void *)&w);

  window = &w;
  if (!plugView)
    return;

  auto platformWindow = window->getNativePlatformWindow();
  if (plugView->isPlatformTypeSupported(platformWindow.type) != kResultTrue) {
    IPlatform::instance().kill(
        -1, std::string("PlugView does not support platform type:") +
                platformWindow.type);
  }

  plugView->setFrame(this);

  if (plugView->attached(platformWindow.ptr, platformWindow.type) !=
      kResultTrue) {
    IPlatform::instance().kill(-1, "Attaching PlugView failed");
  }
}

//------------------------------------------------------------------------
void WindowController::closePlugView() {
  if (plugView) {
    plugView->setFrame(nullptr);
    if (plugView->removed() != kResultTrue) {
      IPlatform::instance().kill(-1, "Removing PlugView failed");
    }
    plugView = nullptr;
  }
  window = nullptr;
}

//------------------------------------------------------------------------
void WindowController::onClose(IWindow &w) {
  SMTG_DBPRT1("onClose called (%p)\n", (void *)&w);

  closePlugView();

  // TODO maybe quit only when the last window is closed
  IPlatform::instance().quit();
}

//------------------------------------------------------------------------
void WindowController::onResize(IWindow &w, Size newSize) {
  SMTG_DBPRT1("onResize called (%p)\n", (void *)&w);

  if (plugView) {
    ViewRect r{};
    r.right = newSize.width;
    r.bottom = newSize.height;
    ViewRect r2{};
    if (plugView->getSize(&r2) == kResultTrue && r != r2)
      plugView->onSize(&r);
  }
}

//------------------------------------------------------------------------
Size WindowController::constrainSize(IWindow &w, Size requestedSize) {
  SMTG_DBPRT1("constrainSize called (%p)\n", (void *)&w);

  if (plugView) {
    ViewRect r{};
    r.right = requestedSize.width;
    r.bottom = requestedSize.height;
    if (plugView->checkSizeConstraint(&r) != kResultTrue) {
      requestedSize.width = r.right - r.left;
      requestedSize.height = r.bottom - r.top;
    }
  }
  return requestedSize;
}

//------------------------------------------------------------------------
void WindowController::onContentScaleFactorChanged(IWindow &w,
                                                   float newScaleFactor) {
  SMTG_DBPRT1("onContentScaleFactorChanged called (%p)\n", (void *)&w);

  if (auto css = U::cast<IPlugViewContentScaleSupport>(plugView)) {
    css->setContentScaleFactor(newScaleFactor);
  }
}

//------------------------------------------------------------------------
tresult PLUGIN_API WindowController::resizeView(IPlugView *view,
                                                ViewRect *newSize) {
  SMTG_DBPRT1("resizeView called (%p)\n", (void *)view);

  if (newSize == nullptr || view == nullptr || view != plugView)
    return kInvalidArgument;
  if (!window)
    return kInternalError;
  if (resizeViewRecursionGard)
    return kResultFalse;
  ViewRect r;
  if (plugView->getSize(&r) != kResultTrue)
    return kInternalError;
  if (r == *newSize)
    return kResultTrue;

  resizeViewRecursionGard = true;
  Size size{newSize->right - newSize->left, newSize->bottom - newSize->top};
  window->resize(size);
  resizeViewRecursionGard = false;
  if (plugView->getSize(&r) != kResultTrue)
    return kInternalError;
  if (r != *newSize)
    plugView->onSize(newSize);
  return kResultTrue;
}

//------------------------------------------------------------------------
} // namespace EditorHost
} // namespace Vst
} // namespace Steinberg
