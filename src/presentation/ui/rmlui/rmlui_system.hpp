#ifndef FOTBILER_RMLUI_SYSTEM_HPP
#define FOTBILER_RMLUI_SYSTEM_HPP

#include <memory>
#include <string>

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

  // Returns true when RmlUi consumed the SDL event.
  bool HandleEvent(SDL_Event& event);

  bool Update();
  bool Render();

private:
  class Impl;
  std::unique_ptr<Impl> impl;
};

}  // namespace blunted::ui

#endif  // FOTBILER_RMLUI_SYSTEM_HPP
