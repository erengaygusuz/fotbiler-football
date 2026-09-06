// Copyright 2019 Google LLC & Bastiaan Konings
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _HPP_GRAPHICS3D_OPENGL
#define _HPP_GRAPHICS3D_OPENGL

#include <SDL2/SDL.h>

#include <algorithm>

#include "interface_renderer3d.hpp"

#ifdef WIN32
#include <SDL2/SDL_syswm.h>
#endif

namespace blunted {

class OpenGLRenderer3D : public Renderer3D {
public:
  OpenGLRenderer3D();
  virtual ~OpenGLRenderer3D();

  virtual void SwapBuffers();
  virtual void SetMatrix(const std::string& shaderUniformName, const Matrix4& matrix);
  virtual void RenderOverlay2D(const std::vector<Overlay2DQueueEntry>& overlay2DQueue);
  virtual void RenderOverlay2D();
  virtual void RenderLights(std::deque<LightQueueEntry>& lightQueue,
                            const Matrix4& projectionMatrix, const Matrix4& viewMatrix);

  virtual bool CreateContext(int width, int height, int bpp, bool fullscreen);

  virtual bool ApplyDisplaySettings(int width, int height, bool fullscreen, bool vsync) override {
    if (!window || width <= 0 || height <= 0) return false;

    const int displayIndex = std::max(0, SDL_GetWindowDisplayIndex(window));
    int result = 0;
    if (fullscreen) {
      SDL_DisplayMode mode{};
      mode.w = width;
      mode.h = height;
      if (SDL_SetWindowDisplayMode(window, &mode) != 0) result = -1;
      if (SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN) != 0) result = -1;
    } else {
      if (SDL_SetWindowFullscreen(window, 0) != 0) result = -1;
      SDL_SetWindowDisplayMode(window, nullptr);
      SDL_SetWindowSize(window, width, height);
      SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex),
                            SDL_WINDOWPOS_CENTERED_DISPLAY(displayIndex));
    }

    SDL_GL_SetSwapInterval(vsync ? 1 : 0);

    int drawableWidth = width;
    int drawableHeight = height;
    SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);
    if (drawableWidth > 0 && drawableHeight > 0) {
      context_width = drawableWidth;
      context_height = drawableHeight;
      SetViewport(0, 0, context_width, context_height);
    }

    // These uniforms were initialized from the original context dimensions.
    // Refresh them after a live resolution/fullscreen change so the next match
    // does not use stale deferred/post-process sampling coordinates.
    const char* contextShaders[] = {"ambient", "lighting", "postprocess"};
    for (const char* shaderName : contextShaders) {
      if (shaders.find(shaderName) == shaders.end()) continue;
      UseShader(shaderName);
      SetUniformFloat(shaderName, "contextX", 0.0f);
      SetUniformFloat(shaderName, "contextY", 0.0f);
      SetUniformFloat(shaderName, "contextWidth", static_cast<float>(context_width));
      SetUniformFloat(shaderName, "contextHeight", static_cast<float>(context_height));
    }
    UseShader("");

    return result == 0;
  }

  virtual void Exit();

  virtual int CreateView(float x_percent, float y_percent, float width_percent,
                         float height_percent);
  virtual View& GetView(int viewID);
  virtual void DeleteView(int viewID);

  virtual void SetCullingMode(e_CullingMode cullingMode);
  virtual void SetBlendingMode(e_BlendingMode blendingMode);
  virtual void SetDepthFunction(e_DepthFunction depthFunction);
  virtual void SetDepthTesting(bool OnOff);
  virtual void SetDepthMask(bool OnOff);
  virtual void SetBlendingFunction(e_BlendingFunction blendingFunction1,
                                   e_BlendingFunction blendingFunction2);
  virtual void SetTextureMode(e_TextureMode textureMode);
  virtual void SetColor(const Vector3& color, float alpha);
  virtual void SetColorMask(bool r, bool g, bool b, bool alpha);

  virtual void ClearBuffer(const Vector3& color, bool clearDepth, bool clearColor);

  virtual Matrix4 CreatePerspectiveMatrix(float aspectRatio, float nearCap = -1, float farCap = -1);
  virtual Matrix4 CreateOrthoMatrix(float left, float right, float bottom, float top,
                                    float nearCap = -1, float farCap = -1);

  virtual VertexBufferID CreateVertexBuffer(float* vertices, unsigned int verticesDataSize,
                                            const std::vector<unsigned int>& indices,
                                            e_VertexBufferUsage usage);
  virtual void UpdateVertexBuffer(VertexBufferID vertexBufferID, float* vertices,
                                  unsigned int verticesDataSize);
  virtual void DeleteVertexBuffer(VertexBufferID vertexBufferID);
  virtual void RenderVertexBuffer(const std::deque<VertexBufferQueueEntry>& vertexBufferQueue,
                                  e_RenderMode renderMode = e_RenderMode_Full);
  virtual void RenderAABB(std::list<VertexBufferQueueEntry>& vertexBufferQueue);
  virtual void RenderAABB(std::list<LightQueueEntry>& lightQueue);

  virtual void SetLight(const Vector3& position, const Vector3& color, float radius);

  virtual int CreateTexture(e_InternalPixelFormat internalPixelFormat, e_PixelFormat pixelFormat,
                            int width, int height, bool alpha = false, bool repeat = true,
                            bool mipmaps = true, bool filter = true, bool multisample = false,
                            bool compareDepth = false);
  virtual void ResizeTexture(int textureID, SDL_Surface* source,
                             e_InternalPixelFormat internalPixelFormat, e_PixelFormat pixelFormat,
                             bool alpha = false, bool mipmaps = true);
  virtual void UpdateTexture(int textureID, SDL_Surface* source, bool alpha = false,
                             bool mipmaps = true);
  virtual void DeleteTexture(int textureID);
  virtual void CopyFrameBufferToTexture(int textureID, int width, int height);
  virtual void BindTexture(int textureID);
  virtual void SetTextureUnit(int textureUnit);
  virtual void SetClientTextureUnit(int textureUnit);

  virtual int CreateFrameBuffer();
  virtual void DeleteFrameBuffer(int fbID);
  virtual void BindFrameBuffer(int fbID);
  virtual void SetFrameBufferRenderBuffer(e_TargetAttachment targetAttachment, int rbID);
  virtual void SetFrameBufferTexture2D(e_TargetAttachment targetAttachment, int texID);
  virtual bool CheckFrameBufferStatus();
  virtual void SetFramebufferGammaCorrection(bool onOff);

  virtual int CreateRenderBuffer();
  virtual void DeleteRenderBuffer(int rbID);
  virtual void BindRenderBuffer(int rbID);
  virtual void SetRenderBufferStorage(e_InternalPixelFormat internalPixelFormat, int width,
                                      int height);

  virtual void SetRenderTargets(const std::vector<e_TargetAttachment>& targetAttachments);

  virtual void SetFOV(float angle);
  virtual void PushAttribute(int attr);
  virtual void PopAttribute();
  virtual void SetViewport(int x, int y, int width, int height);
  virtual void GetContextSize(int& width, int& height, int& bpp);
  virtual void SetPolygonOffset(float scale, float bias);

  virtual void LoadShader(const std::string& name, const std::string& filename);
  virtual void UseShader(const std::string& name);
  virtual void SetUniformInt(const std::string& shaderName, const std::string& varName, int value);
  virtual void SetUniformInt3(const std::string& shaderName, const std::string& varName, int value1,
                              int value2, int value3);
  virtual void SetUniformFloat(const std::string& shaderName, const std::string& varName,
                               float value);
  virtual void SetUniformFloat2(const std::string& shaderName, const std::string& varName,
                                float value1, float value2);
  virtual void SetUniformFloat3(const std::string& shaderName, const std::string& varName,
                                float value1, float value2, float value3);
  virtual void SetUniformFloat3Array(const std::string& shaderName, const std::string& varName,
                                     int count, float* values);
  virtual void SetUniformMatrix4(const std::string& shaderName, const std::string& varName,
                                 const Matrix4& mat);

  virtual void HDRCaptureOverallBrightness();
  virtual float HDRGetOverallBrightness();

  void operator()();

protected:
  SDL_GLContext context;
  SDL_Window* window;
  int context_width, context_height, context_bpp;
  bool contextIsActive;

  float cameraNear;
  float cameraFar;

  int noiseTexID;
  float FOV;
  float overallBrightness;
  float largest_supported_anisotropy;
  void SetMaxAnisotropy();

  std::map<std::string, int> uniformCache;
  std::map<int, int> VBOPingPongMap;
  std::map<int, int> VAOPingPongMap;
  std::map<int, int> VAOReadIndex;

  signed int _cache_activeTextureUnit;

  VertexBufferID overlayBuffer;
  VertexBufferID quadBuffer;
  VertexBufferID CreateSimpleVertexBuffer(float* vertices, unsigned int size);
  void DeleteSimpleVertexBuffer(VertexBufferID vertexBufferID);
  void InitializeOverlayAndQuadBuffers();
};

#ifdef WIN32
static SDL_SysWMinfo wmInfo;
#endif

}  // namespace blunted

#endif
