#ifndef FOTBILER_DISPLAY_MESSAGE_HPP
#define FOTBILER_DISPLAY_MESSAGE_HPP

#include "interface_renderer3d.hpp"

namespace blunted {

class Renderer3DMessage_ApplyFotbilerDisplaySettings : public Command {
public:
  Renderer3DMessage_ApplyFotbilerDisplaySettings(int width, int height, bool fullscreen, bool vsync)
      : Command("r3dmsg_ApplyFotbilerDisplaySettings"),
        width(width),
        height(height),
        fullscreen(fullscreen),
        vsync(vsync) {}

  bool success = false;

protected:
  virtual bool Execute(void* caller = nullptr) override {
    success = static_cast<Renderer3D*>(caller)->ApplyDisplaySettings(
        width, height, fullscreen, vsync);
    return true;
  }

private:
  int width;
  int height;
  bool fullscreen;
  bool vsync;
};

}  // namespace blunted

#endif  // FOTBILER_DISPLAY_MESSAGE_HPP
