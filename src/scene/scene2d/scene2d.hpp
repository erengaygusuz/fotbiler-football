#ifndef _HPP_SCENE2D
#define _HPP_SCENE2D

#include "../object.hpp"
#include "../scene.hpp"
#include "base/properties.hpp"
#include "defines.hpp"
#include "types/lockable.hpp"

namespace blunted {

class Scene2D : public Scene {
public:
  Scene2D(const std::string& name, const Properties& config);
  virtual ~Scene2D();

  virtual void Init();
  virtual void Exit();  // ATOMIC

  void AddObject(boost::intrusive_ptr<Object> object);
  void DeleteObject(boost::intrusive_ptr<Object> object);
  void RemoveObject(boost::intrusive_ptr<Object> object);

  void GetObjects(e_ObjectType targetObjectType,
                  std::vector<boost::intrusive_ptr<Object>>& gatherObjects);
  void PokeObjects(e_ObjectType targetObjectType, e_SystemType targetSystemType);

  void GetContextSize(int& width, int& height, int& bpp);
  Vector3 GetContextSize();

  // The single-process Fotbiler window can change resolution at runtime.
  // Keep the legacy Gui2/HUD canvas aligned with the renderer/RmlUi drawable.
  void SetContextSize(int newWidth, int newHeight, int newBpp) {
    if (newWidth > 0) width = newWidth;
    if (newHeight > 0) height = newHeight;
    if (newBpp > 0) bpp = newBpp;
  }

protected:
  vector_Objects objects;

  int width, height, bpp;
};

class IScene2DInterpreter : public ISceneInterpreter {
public:
  virtual void OnLoad() = 0;
  virtual void OnUnload() = 0;

protected:
};

}  // namespace blunted

#endif