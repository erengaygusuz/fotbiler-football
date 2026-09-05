#include "graphics_system.hpp"

#include <cstdlib>
#include <string>

#include "base/log.hpp"
#include "base/utils.hpp"
#include "graphics_scene.hpp"
#include "managers/resourcemanagerpool.hpp"
#include "rendering/fotbiler_display_message.hpp"
#include "rendering/r3d_messages.hpp"

namespace blunted {

namespace {

bool EnvironmentFlagEnabled(const char* name) {
  const char* value = std::getenv(name);
  return value && value[0] != '\0' && std::string(value) != "0";
}

int EnvironmentInt(const char* name, int fallback) {
  const char* value = std::getenv(name);
  if (!value || value[0] == '\0') {
    return fallback;
  }
  char* end = nullptr;
  const long parsed = std::strtol(value, &end, 10);
  return end && *end == '\0' ? static_cast<int>(parsed) : fallback;
}

}  // namespace

GraphicsSystem::GraphicsSystem() : systemType(e_SystemType_Graphics), fullscreen(false) {
  renderer3DTask = nullptr;
  task = nullptr;
}

GraphicsSystem::~GraphicsSystem() {}

void GraphicsSystem::Initialize(const Properties& config) {
  textureResourceManager =
      std::shared_ptr<ResourceManager<Texture>>(new ResourceManager<Texture>("texture"));
  vertexBufferResourceManager = std::shared_ptr<ResourceManager<VertexBuffer>>(
      new ResourceManager<VertexBuffer>("vertexbuffer"));
  ResourceManagerPool::GetInstance().RegisterManager(e_ResourceType_Texture,
                                                     textureResourceManager);
  ResourceManagerPool::GetInstance().RegisterManager(e_ResourceType_VertexBuffer,
                                                     vertexBufferResourceManager);

  if (config.Get("graphics3d_renderer", "opengl") == "opengl")
    renderer3DTask = new OpenGLRenderer3D();
  width = config.GetInt("context_x", 1280);
  height = config.GetInt("context_y", 720);
  bpp = config.GetInt("context_bpp", 32);
  fullscreen = config.GetBool("context_fullscreen", false);

  if (EnvironmentFlagEnabled("FOTBILER_UI_QUICK_MATCH") ||
      EnvironmentFlagEnabled("FOTBILER_UI_CAREER_MATCH")) {
    fullscreen = true;
  }
  if (std::getenv("FOTBILER_UI_CONTEXT_FULLSCREEN")) {
    fullscreen = EnvironmentFlagEnabled("FOTBILER_UI_CONTEXT_FULLSCREEN");
  }
  width = EnvironmentInt("FOTBILER_UI_CONTEXT_X", width);
  height = EnvironmentInt("FOTBILER_UI_CONTEXT_Y", height);

  renderer3DTask->Run();

  boost::intrusive_ptr<Renderer3DMessage_CreateContext> createContext(
      new Renderer3DMessage_CreateContext(width, height, bpp, fullscreen));
  renderer3DTask->messageQueue.PushMessage(createContext);
  createContext->Wait();

  if (!createContext->success) {
    Log(e_FatalError, "GraphicsSystem", "Initialize", "Could not create context");
  } else {
    // Set swap interval on every platform and refresh the effective drawable
    // dimensions selected by SDL/the compositor.
    const bool startupVsync = config.GetBool("context_vsync", true);
    boost::intrusive_ptr<Renderer3DMessage_ApplyFotbilerDisplaySettings> startupDisplay(
        new Renderer3DMessage_ApplyFotbilerDisplaySettings(width, height, fullscreen, startupVsync));
    renderer3DTask->messageQueue.PushMessage(startupDisplay);
    startupDisplay->Wait();

    boost::intrusive_ptr<Renderer3DMessage_GetContextSize> getContextSize(
        new Renderer3DMessage_GetContextSize());
    renderer3DTask->messageQueue.PushMessage(getContextSize);
    getContextSize->Wait();
    width = getContextSize->GetWidth();
    height = getContextSize->GetHeight();
    bpp = getContextSize->GetBpp();

    Log(e_Notice, "GraphicsSystem", "Initialize",
        "Created context, resolution " + int_to_str(width) + " * " + int_to_str(height) + " @ " +
            int_to_str(bpp) + " bpp" + (fullscreen ? " fullscreen" : " windowed"));
  }

  task = new GraphicsTask(this);
  task->Run();
}

bool GraphicsSystem::ApplyDisplaySettings(int requestedWidth, int requestedHeight,
                                          bool requestedFullscreen, bool vsync) {
  if (!renderer3DTask || requestedWidth <= 0 || requestedHeight <= 0) return false;

  boost::intrusive_ptr<Renderer3DMessage_ApplyFotbilerDisplaySettings> apply(
      new Renderer3DMessage_ApplyFotbilerDisplaySettings(
          requestedWidth, requestedHeight, requestedFullscreen, vsync));
  renderer3DTask->messageQueue.PushMessage(apply);
  apply->Wait();
  if (!apply->success) return false;

  boost::intrusive_ptr<Renderer3DMessage_GetContextSize> getContextSize(
      new Renderer3DMessage_GetContextSize());
  renderer3DTask->messageQueue.PushMessage(getContextSize);
  getContextSize->Wait();

  width = getContextSize->GetWidth();
  height = getContextSize->GetHeight();
  bpp = getContextSize->GetBpp();
  fullscreen = requestedFullscreen;

  Log(e_Notice, "GraphicsSystem", "ApplyDisplaySettings",
      "Applied Fotbiler display settings: " + int_to_str(width) + " * " +
          int_to_str(height) + (fullscreen ? " fullscreen" : " windowed") +
          (vsync ? " vsync" : " no-vsync"));
  return true;
}

void GraphicsSystem::Exit() {
  boost::intrusive_ptr<Message_Shutdown> shutdown(new Message_Shutdown());
  task->messageQueue.PushMessage(shutdown);
  shutdown->Wait();

  task->Join();
  delete task;
  task = nullptr;

  textureResourceManager.reset();
  vertexBufferResourceManager.reset();

  boost::intrusive_ptr<Message_Shutdown> R3Dshutdown(new Message_Shutdown());
  renderer3DTask->messageQueue.PushMessage(R3Dshutdown);
  R3Dshutdown->Wait();

  renderer3DTask->Join();
  delete renderer3DTask;
  renderer3DTask = nullptr;
}

e_SystemType GraphicsSystem::GetSystemType() const {
  return systemType;
}

ISystemScene* GraphicsSystem::CreateSystemScene(std::shared_ptr<IScene> scene) {
  if (scene->GetSceneType() == e_SceneType_Scene2D) {
    GraphicsScene* graphicsScene = new GraphicsScene(this);
    scene->Attach(graphicsScene->GetInterpreter(e_SceneType_Scene2D));
    return graphicsScene;
  }
  if (scene->GetSceneType() == e_SceneType_Scene3D) {
    GraphicsScene* graphicsScene = new GraphicsScene(this);
    scene->Attach(graphicsScene->GetInterpreter(e_SceneType_Scene3D));
    return graphicsScene;
  }
  return nullptr;
}

ISystemTask* GraphicsSystem::GetTask() {
  return task;
}

Renderer3D* GraphicsSystem::GetRenderer3D() {
  return renderer3DTask;
}

MessageQueue<Overlay2DQueueEntry>& GraphicsSystem::GetOverlay2DQueue() {
  return overlay2DQueue;
}

}  // namespace blunted