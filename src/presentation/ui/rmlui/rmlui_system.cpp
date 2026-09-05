#include "rmlui_system.hpp"

#include <memory>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/ElementDocument.h>

#include "RmlUi_Platform_SDL.h"
#include "RmlUi_Renderer_GL3.h"

namespace blunted::ui {

class RmlUiSystem::Impl {
public:
  SDL_Window* window = nullptr;
  std::unique_ptr<SystemInterface_SDL> systemInterface;
  std::unique_ptr<RenderInterface_GL3> renderInterface;
  std::unique_ptr<TextInputMethodEditor_SDL> textInputHandler;
  Rml::Context* context = nullptr;
  Rml::ElementDocument* document = nullptr;
  bool glInitialized = false;
  bool rmlInitialized = false;
};

RmlUiSystem::RmlUiSystem() : impl(std::make_unique<Impl>()) {}

RmlUiSystem::~RmlUiSystem() {
  Shutdown();
}

bool RmlUiSystem::Initialize(SDL_Window* window, int width, int height) {
  if (impl->context) {
    return true;
  }
  if (!window || width <= 0 || height <= 0) {
    return false;
  }

  Rml::String glMessage;
  if (!RmlGL3::Initialize(&glMessage)) {
    return false;
  }
  impl->glInitialized = true;
  impl->window = window;

  impl->systemInterface = std::make_unique<SystemInterface_SDL>(window);
  impl->renderInterface = std::make_unique<RenderInterface_GL3>();
  impl->textInputHandler = std::make_unique<TextInputMethodEditor_SDL>();

  if (!static_cast<bool>(*impl->renderInterface)) {
    Shutdown();
    return false;
  }

  Rml::SetSystemInterface(impl->systemInterface.get());
  Rml::SetRenderInterface(impl->renderInterface.get());
  Rml::SetTextInputHandler(impl->textInputHandler.get());

  if (!Rml::Initialise()) {
    Shutdown();
    return false;
  }
  impl->rmlInitialized = true;

  impl->context = Rml::CreateContext("fotbiler", Rml::Vector2i(width, height));
  if (!impl->context) {
    Shutdown();
    return false;
  }

  impl->renderInterface->SetViewport(width, height);
  return true;
}

void RmlUiSystem::Shutdown() {
  if (!impl) {
    return;
  }

  impl->document = nullptr;
  if (impl->context) {
    const Rml::String contextName = impl->context->GetName();
    Rml::RemoveContext(contextName);
    impl->context = nullptr;
  }

  if (impl->rmlInitialized) {
    Rml::Shutdown();
    impl->rmlInitialized = false;
  }

  impl->textInputHandler.reset();
  impl->renderInterface.reset();
  impl->systemInterface.reset();

  if (impl->glInitialized) {
    RmlGL3::Shutdown();
    impl->glInitialized = false;
  }

  impl->window = nullptr;
}

bool RmlUiSystem::IsInitialized() const {
  return impl && impl->context;
}

void RmlUiSystem::SetDimensions(int width, int height) {
  if (!impl->context || width <= 0 || height <= 0) {
    return;
  }

  impl->context->SetDimensions(Rml::Vector2i(width, height));
  impl->renderInterface->SetViewport(width, height);
}

bool RmlUiSystem::LoadDocument(const std::string& path) {
  if (!impl->context || path.empty()) {
    return false;
  }

  UnloadDocument();
  impl->document = impl->context->LoadDocument(path);
  if (!impl->document) {
    return false;
  }

  impl->document->Show();
  return true;
}

void RmlUiSystem::UnloadDocument() {
  if (!impl->context || !impl->document) {
    return;
  }

  impl->context->UnloadDocument(impl->document);
  impl->document = nullptr;
}

bool RmlUiSystem::HasDocument() const {
  return impl && impl->document;
}

bool RmlUiSystem::HandleEvent(SDL_Event& event) {
  if (!impl->context || !impl->window) {
    return false;
  }

  return !RmlSDL::InputEventHandler(impl->context, impl->window, event);
}

bool RmlUiSystem::Update() {
  return impl->context && impl->context->Update();
}

bool RmlUiSystem::Render() {
  if (!impl->context || !impl->renderInterface) {
    return false;
  }

  impl->renderInterface->BeginFrame();
  const bool rendered = impl->context->Render();
  impl->renderInterface->EndFrame();
  return rendered;
}

}  // namespace blunted::ui
