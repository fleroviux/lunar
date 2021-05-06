#include <algorithm>
#include <fstream>
#include <memory>
#include <stdio.h>
#include <util/integer.hpp>
#include <util/log.hpp>
#include <core/core.hpp>
#include <duality/emulator_thread.hpp>

#include "device/audio_device.hpp"
#include "device/video_device.hpp"

#undef main

void loop(const char* rom_path) {
  SDL_Init(SDL_INIT_VIDEO);

  auto scale = 2;
  auto window = SDL_CreateWindow(
    "Duality",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    256 * scale,
    384 * scale,
    SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

  auto gl_context = SDL_GL_CreateContext(window);

  glewInit();

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetSwapInterval(1);

  auto audio_device = SDL2AudioDevice{};
  auto input_device = Duality::Core::BasicInputDevice{};
  auto video_device = SDL2VideoDevice{window};
  auto core = Duality::Core::Core{rom_path};

  core.SetAudioDevice(audio_device);
  core.SetInputDevice(input_device);
  core.SetVideoDevice(video_device);

  int frames = 0;
  auto t0 = SDL_GetTicks();
  SDL_Event event;

  auto emu_thread = Duality::EmulatorThread{core};
  emu_thread.Start();

  for (;;) {
    auto fps = emu_thread.GetFPS();
    SDL_SetWindowTitle(window, fmt::format("Duality [{0:.2f} fps | {1:.2f} ms]",
        fps, 1000.0 / fps).c_str());

    video_device.Present();

    while (SDL_PollEvent(&event)) {
      using Key = Duality::Core::InputDevice::Key;

      if (event.type == SDL_QUIT)
        goto cleanup;

      if (event.type == SDL_KEYUP || event.type == SDL_KEYDOWN) {
        int key = -1;
        bool down = event.type == SDL_KEYDOWN;

        switch (reinterpret_cast<SDL_KeyboardEvent*>(&event)->keysym.sym) {
          case SDLK_a: input_device.SetKeyDown(Key::A, down); break;
          case SDLK_s: input_device.SetKeyDown(Key::B, down); break;
          case SDLK_BACKSPACE: input_device.SetKeyDown(Key::Select, down); break;
          case SDLK_RETURN: input_device.SetKeyDown(Key::Start, down); break;
          case SDLK_RIGHT: input_device.SetKeyDown(Key::Right, down); break;
          case SDLK_LEFT: input_device.SetKeyDown(Key::Left, down); break;
          case SDLK_UP: input_device.SetKeyDown(Key::Up, down); break;
          case SDLK_DOWN: input_device.SetKeyDown(Key::Down, down); break;
          case SDLK_d: input_device.SetKeyDown(Key::L, down); break;
          case SDLK_f: input_device.SetKeyDown(Key::R, down); break;
          case SDLK_q: input_device.SetKeyDown(Key::X, down); break;
          case SDLK_w: input_device.SetKeyDown(Key::Y, down); break;
          case SDLK_SPACE: emu_thread.SetFastForward(down); break;
        }
      }

      if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
        auto mouse_event = reinterpret_cast<SDL_MouseMotionEvent*>(&event);
        int x = mouse_event->x / scale;
        int y = mouse_event->y / scale - 192;
        bool down = (mouse_event->state & SDL_BUTTON_LMASK) && y >= 0;
        input_device.SetKeyDown(Key::TouchPen, down);
        input_device.GetTouchPoint().x = x;
        input_device.GetTouchPoint().y = y;
      }
    }
  }

cleanup:
  emu_thread.Stop();
  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
}

auto main(int argc, const char** argv) -> int {
  if (argc != 2) {
    printf("%s rom_path\n", argv[0]);
    return -1;
  }
  loop(argv[1]);
  return 0;
}
