// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is part of the examples of the Open CASCADE Technology software library.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE

#include "GlfwOcctWindow.h"

#if defined (__APPLE__)
  #undef Handle // avoid name collisions in macOS headers
  #define GLFW_EXPOSE_NATIVE_COCOA
  #define GLFW_EXPOSE_NATIVE_NSGL
#elif defined (_WIN32)
  #define GLFW_EXPOSE_NATIVE_WIN32
  #define GLFW_EXPOSE_NATIVE_WGL
#elif defined(HAVE_EGL) || defined(HAVE_WAYLAND)
  // GLFW3 could be built either with X11 or Wayland, but not for both at once.
  // Linux distros provide two exclusive packages 'libglfw3' and 'libglfw3-wayland'
  // both installing the library with exactly same SONAME.
  //
  // When OCCT is built with EGL support (which is a requirement for Wayland)
  // as well as X11 (USE_XLIB=ON) + Wayland (USE_WAYLAND=ON),
  // both GLFW3 implementations will also work (through EGL layer).
  //#if defined(HAVE_WAYLAND)
  //#define GLFW_EXPOSE_NATIVE_WAYLAND
  //#endif
  #define GLFW_EXPOSE_NATIVE_EGL
#else
  #define GLFW_EXPOSE_NATIVE_X11
  #define GLFW_EXPOSE_NATIVE_GLX
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#if !defined(_WIN32) && !defined(__APPLE__) && !defined(HAVE_EGL) && !defined(HAVE_WAYLAND)
  #include <Xw_DisplayConnection.hxx>
#endif

// ================================================================
// Function : GlfwOcctWindow
// Purpose  :
// ================================================================
GlfwOcctWindow::GlfwOcctWindow (int theWidth, int theHeight, const TCollection_AsciiString& theTitle)
: myGlfwWindow (glfwCreateWindow (theWidth, theHeight, theTitle.ToCString(), NULL, NULL)),
  myXLeft  (0),
  myYTop   (0),
  myXRight (0),
  myYBottom(0)
{
  if (myGlfwWindow != nullptr)
  {
    int aWidth = 0, aHeight = 0;
    // glfwGetWindowPos() prints error within Wayland backend
    //#if !defined(HAVE_WAYLAND)
    glfwGetWindowPos (myGlfwWindow, &myXLeft, &myYTop);
    //#endif
    glfwGetWindowSize(myGlfwWindow, &aWidth, &aHeight);
    myXRight  = myXLeft + aWidth;
    myYBottom = myYTop + aHeight;

  #if !defined(_WIN32) && !defined(__APPLE__) && !defined(HAVE_EGL) && !defined(HAVE_WAYLAND)
    myDisplay = new Xw_DisplayConnection ((Aspect_XDisplay* )glfwGetX11Display());
  #endif
  }
}

// ================================================================
// Function : Close
// Purpose  :
// ================================================================
void GlfwOcctWindow::Close()
{
  if (myGlfwWindow != nullptr)
  {
    glfwDestroyWindow (myGlfwWindow);
    myGlfwWindow = nullptr;
  }
}

// ================================================================
// Function : NativeHandle
// Purpose  :
// ================================================================
Aspect_Drawable GlfwOcctWindow::NativeHandle() const
{
#if defined (__APPLE__)
  return (Aspect_Drawable)glfwGetCocoaWindow (myGlfwWindow);
#elif defined (_WIN32)
  return (Aspect_Drawable)glfwGetWin32Window (myGlfwWindow);
#elif defined(HAVE_EGL) || defined(HAVE_WAYLAND)
  // OCCT expects wl_egl_window in case of Wayland (inaccessible from GLFW)
  // and Window in case of X11, but this handle will be unused anyway
  return (Aspect_Drawable)glfwGetEGLSurface (myGlfwWindow);
#else
  return (Aspect_Drawable)glfwGetX11Window (myGlfwWindow);
#endif
}

// ================================================================
// Function : NativeGlContext
// Purpose  :
// ================================================================
Aspect_RenderingContext GlfwOcctWindow::NativeGlContext() const
{
#if defined (__APPLE__)
  return (NSOpenGLContext*)glfwGetNSGLContext (myGlfwWindow);
#elif defined (_WIN32)
  return glfwGetWGLContext (myGlfwWindow);
#elif defined(HAVE_EGL) || defined(HAVE_WAYLAND)
  return glfwGetEGLContext (myGlfwWindow);
#else
  return glfwGetGLXContext (myGlfwWindow);
#endif
}

// ================================================================
// Function : NativeEglDisplay
// Purpose  :
// ================================================================
Aspect_Display GlfwOcctWindow::NativeEglDisplay() const
{
#if defined(HAVE_EGL) || defined(HAVE_WAYLAND)
  return Aspect_Display (glfwGetEGLDisplay());
#else
  return 0;
#endif
}

// ================================================================
// Function : IsMapped
// Purpose  :
// ================================================================
Standard_Boolean GlfwOcctWindow::IsMapped() const
{
  return glfwGetWindowAttrib (myGlfwWindow, GLFW_VISIBLE) != 0;
}

// ================================================================
// Function : Map
// Purpose  :
// ================================================================
void GlfwOcctWindow::Map() const
{
  glfwShowWindow (myGlfwWindow);
}

// ================================================================
// Function : Unmap
// Purpose  :
// ================================================================
void GlfwOcctWindow::Unmap() const
{
  glfwHideWindow (myGlfwWindow);
}

// ================================================================
// Function : DoResize
// Purpose  :
// ================================================================
Aspect_TypeOfResize GlfwOcctWindow::DoResize()
{
  if (glfwGetWindowAttrib (myGlfwWindow, GLFW_VISIBLE) == 1)
  {
    int anXPos = 0, anYPos = 0, aWidth = 0, aHeight = 0;
    // glfwGetWindowPos() prints error within Wayland backend
    //#if !defined(HAVE_WAYLAND)
    glfwGetWindowPos (myGlfwWindow, &anXPos, &anYPos);
    //#endif
    glfwGetWindowSize(myGlfwWindow, &aWidth, &aHeight);
    myXLeft   = anXPos;
    myXRight  = anXPos + aWidth;
    myYTop    = anYPos;
    myYBottom = anYPos + aHeight;
  }
  return Aspect_TOR_UNKNOWN;
}

// ================================================================
// Function : CursorPosition
// Purpose  :
// ================================================================
Graphic3d_Vec2i GlfwOcctWindow::CursorPosition() const
{
  Graphic3d_Vec2d aPos;
  glfwGetCursorPos (myGlfwWindow, &aPos.x(), &aPos.y());
  return Graphic3d_Vec2i ((int )aPos.x(), (int )aPos.y());
}
