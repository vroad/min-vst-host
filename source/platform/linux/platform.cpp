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

#include "source/platform/iplatform.h"
#include "source/platform/linux/irunloopimpl.h"
#include "source/platform/linux/window.h"

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {
namespace EditorHost {

using namespace std::chrono;
using clock = high_resolution_clock;

//------------------------------------------------------------------------
static int pause(int milliSeconds) {
  struct timeval timeOut;
  if (milliSeconds > 0) {
    timeOut.tv_usec = (milliSeconds % (unsigned long)1000) * 1000;
    timeOut.tv_sec = milliSeconds / (unsigned long)1000;

    select(0, nullptr, nullptr, nullptr, &timeOut);
  }
  return (milliSeconds > 0 ? milliSeconds : 0);
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
class Platform : public IPlatform {
public:
  static Platform &instance() {
    static Platform gInstance;
    return gInstance;
  }

  void setApplication(ApplicationPtr &&app) override;
  WindowPtr createWindow(const std::string &title, Size size, bool resizeable,
                         const WindowControllerPtr &controller) override;

  void quit() override;
  void kill(int resultCode, const std::string &reason) override;

  FUnknown *getPluginFactoryContext() override;

  void run(const std::vector<std::string> &cmdArgs);

  static const int kMinEventLoopRate = 16; // 60Hz
private:
  void onWindowClosed(X11Window *window);
  void closeAllWindows();
  void eventLoop();

  ApplicationPtr application;
  Display *xDisplay{nullptr};

  std::vector<std::shared_ptr<X11Window>> windows;
};

//------------------------------------------------------------------------
IPlatform &IPlatform::instance() { return Platform::instance(); }

//------------------------------------------------------------------------
void Platform::setApplication(ApplicationPtr &&app) {
  application = std::move(app);
}

//------------------------------------------------------------------------
WindowPtr Platform::createWindow(const std::string &title, Size size,
                                 bool resizeable,
                                 const WindowControllerPtr &controller) {
  auto window =
      X11Window::make(title, size, resizeable, controller, xDisplay,
                      [this](X11Window *window) { onWindowClosed(window); });
  if (window)
    windows.push_back(std::static_pointer_cast<X11Window>(window));
  return window;
}

//------------------------------------------------------------------------
void Platform::onWindowClosed(X11Window *window) {
  for (auto it = windows.begin(); it != windows.end(); ++it) {
    if (it->get() == window) {
      windows.erase(it);
      break;
    }
  }
}

//------------------------------------------------------------------------
void Platform::closeAllWindows() {
  for (auto &w : windows) {
    w->close();
  }
}

//------------------------------------------------------------------------
void Platform::quit() {
  static bool recursiveGuard = false;
  if (recursiveGuard)
    return;
  recursiveGuard = true;

  closeAllWindows();

  if (application)
    application->terminate();

  RunLoop::instance().stop();

  recursiveGuard = false;
}

//------------------------------------------------------------------------
void Platform::kill(int resultCode, const std::string &reason) {
  std::cout << reason << "\n";
  exit(resultCode);
}

//------------------------------------------------------------------------
FUnknown *Platform::getPluginFactoryContext() {
  return &Steinberg::Linux::RunLoopImpl::instance();
}

//------------------------------------------------------------------------
void Platform::run(const std::vector<std::string> &cmdArgs) {
  // Connect to X server
  std::string displayName(getenv("DISPLAY"));
  if (displayName.empty())
    displayName = ":0.0";

  if ((xDisplay = XOpenDisplay(displayName.data())) == nullptr) {
    return;
  }

  RunLoop::instance().setDisplay(xDisplay);

  application->init(cmdArgs);

  eventLoop();

  XCloseDisplay(xDisplay);
}

//------------------------------------------------------------------------
void Platform::eventLoop() { RunLoop::instance().start(); }

//------------------------------------------------------------------------
} // namespace EditorHost
} // namespace Vst
} // namespace Steinberg

//------------------------------------------------------------------------
//------------------------------------------------------------------------
int main(int argc, char *argv[]) {
  std::vector<std::string> cmdArgs;
  for (int i = 1; i < argc; ++i)
    cmdArgs.push_back(argv[i]);

  Steinberg::Vst::EditorHost::Platform::instance().run(cmdArgs);

  return 0;
}
