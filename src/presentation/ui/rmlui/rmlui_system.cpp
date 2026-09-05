#include "rmlui_system.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>

#include "RmlUi_Platform_SDL.h"
#include "RmlUi_Renderer_GL3.h"

namespace blunted::ui {
namespace {

std::string GetExecutableAssetPath(const char* relativePath) {
  char* basePath = SDL_GetBasePath();
  std::string path = basePath ? basePath : "";
  SDL_free(basePath);
  path += relativePath;
  return path;
}

bool LoadFotbilerFonts() {
  const std::string sansPath =
      GetExecutableAssetPath("media/fonts/alegreya/AlegreyaSans-ExtraBold.ttf");
  const std::string displayPath =
      GetExecutableAssetPath("media/fonts/alegreya/AlegreyaSansSC-Black.ttf");

  const bool sansLoaded =
      Rml::LoadFontFace(sansPath, "Fotbiler Sans", Rml::Style::FontStyle::Normal,
                        Rml::Style::FontWeight::Normal);
  const bool displayLoaded =
      Rml::LoadFontFace(displayPath, "Fotbiler Display", Rml::Style::FontStyle::Normal,
                        Rml::Style::FontWeight::Normal);
  return sansLoaded && displayLoaded;
}

class InteractionEventListener final : public Rml::EventListener {
public:
  InteractionEventListener(std::string& pendingRoute, std::string& pendingAction)
      : pendingRoute(pendingRoute), pendingAction(pendingAction) {}

  void ProcessEvent(Rml::Event& event) override {
    Rml::Element* element = event.GetTargetElement();
    while (element) {
      const Rml::String action =
          element->GetAttribute<Rml::String>("data-action", Rml::String());
      if (!action.empty()) {
        pendingAction = action;
        return;
      }

      const Rml::String route =
          element->GetAttribute<Rml::String>("data-route", Rml::String());
      if (!route.empty()) {
        pendingRoute = route;
        return;
      }
      element = element->GetParentNode();
    }
  }

private:
  std::string& pendingRoute;
  std::string& pendingAction;
};

}  // namespace

class RmlUiSystem::Impl {
public:
  SDL_Window* window = nullptr;
  std::unique_ptr<SystemInterface_SDL> systemInterface;
  std::unique_ptr<RenderInterface_GL3> renderInterface;
  std::unique_ptr<TextInputMethodEditor_SDL> textInputHandler;
  std::unique_ptr<InteractionEventListener> interactionEventListener;
  Rml::Context* context = nullptr;
  Rml::ElementDocument* document = nullptr;
  std::string pendingRoute;
  std::string pendingAction;
  std::string currentDocumentPath;
  std::unordered_map<std::string, std::string> focusByDocument;
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

  if (!LoadFotbilerFonts()) {
    Shutdown();
    return false;
  }

  impl->context = Rml::CreateContext("fotbiler", Rml::Vector2i(width, height));
  if (!impl->context) {
    Shutdown();
    return false;
  }

  impl->interactionEventListener =
      std::make_unique<InteractionEventListener>(impl->pendingRoute, impl->pendingAction);
  impl->context->AddEventListener("click", impl->interactionEventListener.get());
  impl->renderInterface->SetViewport(width, height);
  return true;
}

void RmlUiSystem::Shutdown() {
  if (!impl) {
    return;
  }

  impl->document = nullptr;
  if (impl->context) {
    if (impl->interactionEventListener) {
      impl->context->RemoveEventListener("click", impl->interactionEventListener.get());
    }
    impl->interactionEventListener.reset();

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

  impl->pendingRoute.clear();
  impl->pendingAction.clear();
  impl->currentDocumentPath.clear();
  impl->focusByDocument.clear();
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

  impl->currentDocumentPath = path;
  impl->document->Show();
  FocusDefaultElement();
  return true;
}

void RmlUiSystem::UnloadDocument() {
  if (!impl->context || !impl->document) {
    return;
  }

  if (!impl->currentDocumentPath.empty()) {
    Rml::Element* focused = impl->document->GetFocusLeafNode();
    if (focused && focused != impl->document && !focused->GetId().empty()) {
      impl->focusByDocument[impl->currentDocumentPath] = focused->GetId();
    }
  }

  impl->context->UnloadDocument(impl->document);
  impl->document = nullptr;
  impl->currentDocumentPath.clear();
}

bool RmlUiSystem::HasDocument() const {
  return impl && impl->document;
}

bool RmlUiSystem::SetElementText(const std::string& elementId, const std::string& text) {
  if (!impl || !impl->document || elementId.empty()) {
    return false;
  }

  Rml::Element* element = impl->document->GetElementById(elementId);
  if (!element) {
    return false;
  }

  element->SetInnerRML(text);
  return true;
}

bool RmlUiSystem::HandleEvent(SDL_Event& event) {
  if (!impl->context || !impl->window) {
    return false;
  }

  return !RmlSDL::InputEventHandler(impl->context, impl->window, event);
}

std::string RmlUiSystem::ConsumeRouteRequest() {
  if (!impl) {
    return {};
  }

  std::string request = std::move(impl->pendingRoute);
  impl->pendingRoute.clear();
  return request;
}

std::string RmlUiSystem::ConsumeActionRequest() {
  if (!impl) {
    return {};
  }

  std::string request = std::move(impl->pendingAction);
  impl->pendingAction.clear();
  return request;
}

bool RmlUiSystem::ActivateFocusedElement() {
  if (!impl || !impl->document) {
    return false;
  }

  Rml::Element* focused = impl->document->GetFocusLeafNode();
  if (!focused || focused == impl->document) {
    return false;
  }

  focused->Click();
  return true;
}

bool RmlUiSystem::FocusDefaultElement() {
  if (!impl || !impl->document) {
    return false;
  }

  if (!impl->currentDocumentPath.empty()) {
    const auto remembered = impl->focusByDocument.find(impl->currentDocumentPath);
    if (remembered != impl->focusByDocument.end()) {
      if (Rml::Element* element = impl->document->GetElementById(remembered->second)) {
        if (element->Focus(true)) {
          return true;
        }
      }
    }
  }

  const char* selectors[] = {"[autofocus]", "[data-route]", "[data-action]", "[tabindex]"};
  for (const char* selector : selectors) {
    if (Rml::Element* element = impl->document->QuerySelector(selector)) {
      if (element->Focus(true)) {
        return true;
      }
    }
  }

  return false;
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
