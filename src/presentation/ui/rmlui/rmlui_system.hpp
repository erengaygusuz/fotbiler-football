#ifndef FOTBILER_RMLUI_SYSTEM_HPP
#define FOTBILER_RMLUI_SYSTEM_HPP

#include <memory>
#include <string>

#include "presentation/ui/rmlui/ui_action.hpp"

struct SDL_Window;
union SDL_Event;

namespace blunted::ui {

// Owns Fotbiler's RmlUi context and the SDL2/OpenGL3 backend interfaces.
// This is deliberately independent from Gui2 so legacy screens can coexist
// while new player-facing screens migrate incrementally.
class RmlUiSystem {
public:
  RmlUiSystem();
  ~RmlUiSystem();

  RmlUiSystem(const RmlUiSystem&) = delete;
  RmlUiSystem& operator=(const RmlUiSystem&) = delete;

  bool Initialize(SDL_Window* window, int width, int height);
  void Shutdown();

  bool IsInitialized() const;
  void SetDimensions(int width, int height);

  bool LoadDocument(const std::string& path);
  void UnloadDocument();
  bool HasDocument() const;

  // Presentation binding primitives. They return false when the current
  // document does not contain the requested id, allowing binders and modal
  // controllers to stay screen-agnostic.
  bool SetElementText(const std::string& elementId, const std::string& text);
  bool SetElementProperty(const std::string& elementId, const std::string& property,
                          const std::string& value);
  bool FocusElement(const std::string& elementId);

  // Returns true when RmlUi consumed the SDL event.
  bool HandleEvent(SDL_Event& event);

  // data-route clicks are queued so callers can switch documents safely after
  // the current RmlUi event dispatch has completed.
  std::string ConsumeRouteRequest();

  // Structured player-facing commands. A document can supply optional
  // data-action-args="key=value;..." payload alongside data-action.
  UiAction ConsumeAction();

  // Compatibility helper for the already-wired parameterless frontend actions.
  // New feature-parity work should prefer ConsumeAction().
  std::string ConsumeActionRequest();

  // Activates the current focus target and restores a deterministic default
  // focus when a screen is loaded without a remembered focus target.
  bool ActivateFocusedElement();
  bool FocusDefaultElement();

  bool Update();
  bool Render();

private:
  class Impl;
  std::unique_ptr<Impl> impl;
};

}  // namespace blunted::ui

#endif  // FOTBILER_RMLUI_SYSTEM_HPP
