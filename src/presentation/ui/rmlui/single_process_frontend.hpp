#ifndef FOTBILER_SINGLE_PROCESS_FRONTEND_HPP
#define FOTBILER_SINGLE_PROCESS_FRONTEND_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "presentation/ui/rmlui/frontend_runtime_bridge.hpp"
#include "presentation/ui/rmlui/screen_router.hpp"

union SDL_Event;

namespace blunted::ui {

class RmlUiSystem;

class SingleProcessFrontend {
public:
  explicit SingleProcessFrontend(RmlUiSystem& ui);
  ~SingleProcessFrontend();

  SingleProcessFrontend(const SingleProcessFrontend&) = delete;
  SingleProcessFrontend& operator=(const SingleProcessFrontend&) = delete;

  bool Initialize();
  void Shutdown();

  bool HandleEvent(SDL_Event& event);
  bool UpdateAndRender();

  void SuspendForMatch();
  bool Resume(frontend::ReturnTarget target);

  bool IsInitialized() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl;
};

}  // namespace blunted::ui

#endif  // FOTBILER_SINGLE_PROCESS_FRONTEND_HPP
