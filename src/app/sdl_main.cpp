#include "content_pack.hpp"
#include "frontend.hpp"
#include "movement.hpp"
#include "zoom_zoo_pack.hpp"
#include "presentation.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
using unirally::app::GamepadButton;
using unirally::app::KeyboardKey;

template <typename T, void (*Destroy)(T *)> struct SdlDeleter {
  void operator()(T *value) const { Destroy(value); }
};
using Window = std::unique_ptr<SDL_Window, SdlDeleter<SDL_Window, SDL_DestroyWindow>>;
using Renderer =
    std::unique_ptr<SDL_Renderer, SdlDeleter<SDL_Renderer, SDL_DestroyRenderer>>;
using Texture =
    std::unique_ptr<SDL_Texture, SdlDeleter<SDL_Texture, SDL_DestroyTexture>>;
using Gamepad =
    std::unique_ptr<SDL_Gamepad, SdlDeleter<SDL_Gamepad, SDL_CloseGamepad>>;

struct SdlQuitter {
  ~SdlQuitter() { SDL_Quit(); }
};

std::runtime_error sdl_error(std::string_view operation) {
  return std::runtime_error(std::string(operation) + ": " + SDL_GetError());
}

std::optional<KeyboardKey> keyboard_key(SDL_Scancode key) {
  switch (key) {
  case SDL_SCANCODE_Z: return KeyboardKey::Z;
  case SDL_SCANCODE_X: return KeyboardKey::X;
  case SDL_SCANCODE_BACKSPACE: return KeyboardKey::Backspace;
  case SDL_SCANCODE_RETURN: return KeyboardKey::Return;
  case SDL_SCANCODE_UP: return KeyboardKey::Up;
  case SDL_SCANCODE_DOWN: return KeyboardKey::Down;
  case SDL_SCANCODE_LEFT: return KeyboardKey::Left;
  case SDL_SCANCODE_RIGHT: return KeyboardKey::Right;
  case SDL_SCANCODE_A: return KeyboardKey::A;
  case SDL_SCANCODE_S: return KeyboardKey::S;
  case SDL_SCANCODE_Q: return KeyboardKey::Q;
  case SDL_SCANCODE_W: return KeyboardKey::W;
  default: return std::nullopt;
  }
}

std::optional<GamepadButton> gamepad_button(Uint8 button) {
  switch (button) {
  case SDL_GAMEPAD_BUTTON_SOUTH: return GamepadButton::South;
  case SDL_GAMEPAD_BUTTON_WEST: return GamepadButton::West;
  case SDL_GAMEPAD_BUTTON_BACK: return GamepadButton::Back;
  case SDL_GAMEPAD_BUTTON_START: return GamepadButton::Start;
  case SDL_GAMEPAD_BUTTON_DPAD_UP: return GamepadButton::DpadUp;
  case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return GamepadButton::DpadDown;
  case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return GamepadButton::DpadLeft;
  case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return GamepadButton::DpadRight;
  case SDL_GAMEPAD_BUTTON_EAST: return GamepadButton::East;
  case SDL_GAMEPAD_BUTTON_NORTH: return GamepadButton::North;
  case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return GamepadButton::LeftShoulder;
  case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return GamepadButton::RightShoulder;
  default: return std::nullopt;
  }
}

struct OpenGamepad {
  SDL_JoystickID id{};
  Gamepad handle;
};

class Gamepads {
public:
  explicit Gamepads(unirally::app::InputState &input) : input_(input) {}

  void added(SDL_JoystickID id) {
    if (port_for(id))
      return;
    const auto empty = std::find_if(slots_.begin(), slots_.end(),
                                    [](const auto &slot) { return !slot; });
    if (empty == slots_.end())
      return;
    Gamepad opened(SDL_OpenGamepad(id));
    if (!opened)
      throw sdl_error("cannot open gamepad");
    const auto port = static_cast<std::size_t>(empty - slots_.begin());
    *empty = OpenGamepad{id, std::move(opened)};
    std::cout << "Gamepad connected to controller port " << port << '\n';
  }

  void removed(SDL_JoystickID id) {
    const auto port = port_for(id);
    if (!port)
      return;
    input_.disconnect(*port);
    slots_[*port].reset();
    std::cout << "Gamepad removed from controller port " << unsigned(*port)
              << '\n';
  }

  void button(SDL_JoystickID id, Uint8 button, bool pressed) {
    const auto port = port_for(id);
    const auto mapped = gamepad_button(button);
    if (port && mapped)
      input_.gamepad(*port, *mapped, pressed);
  }

private:
  std::optional<std::uint8_t> port_for(SDL_JoystickID id) const {
    for (std::uint8_t port = 0; port < slots_.size(); ++port)
      if (slots_[port] && slots_[port]->id == id)
        return port;
    return std::nullopt;
  }
  unirally::app::InputState &input_;
  std::array<std::optional<OpenGamepad>, 2> slots_{};
};

struct Options {
  std::filesystem::path pack;
  std::uint32_t maximum_updates{};
  std::optional<std::uint16_t> fixed_controller_mask;
  bool hidden{};
  bool zoom_zoo{};
};

std::uint32_t parse_updates(std::string_view value) {
  std::uint32_t parsed{};
  const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
      parsed == 0)
    throw std::invalid_argument("--updates requires a positive integer");
  return parsed;
}

std::uint16_t parse_controller_mask(std::string_view value) {
  unsigned parsed{};
  const auto result =
      std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size() ||
      parsed > 0xffffU)
    throw std::invalid_argument(
        "--fixed-controller-mask requires an integer from 0 through 65535");
  return static_cast<std::uint16_t>(parsed);
}

void print_help() {
  std::cout
      << "Usage: unirally --content-pack PATH [--track dragster|zoom-zoo] [--updates N] [--hidden]\n"
      << "Runs the Classic CRAWLER / DRAGSTER native slice at PAL 50 Hz.\n"
      << "Keyboard: arrows, Z=B, X=Y, A=A, S=X, Q=L, W=R, Enter=Start.\n"
      << "Two standard gamepads are tracked; this slice consumes port 0 only.\n"
      << "Audio is intentionally not implemented in M3.\n";
}

std::optional<Options> options(int argc, char **argv) {
  Options result;
  for (int index = 1; index < argc; ++index) {
    const std::string_view option(argv[index]);
    if (option == "--help" || option == "-h") {
      print_help();
      return std::nullopt;
    }
    if (option == "--hidden") {
      result.hidden = true;
      continue;
    }
    if (index + 1 >= argc)
      throw std::invalid_argument(std::string(option) + " requires a value");
    const std::string_view value(argv[++index]);
    if (option == "--track") {
      if(value!="dragster" && value!="zoom-zoo")throw std::invalid_argument("unknown track");
      result.zoom_zoo=value=="zoom-zoo";
    } else if (option == "--content-pack")
      result.pack = value;
    else if (option == "--updates")
      result.maximum_updates = parse_updates(value);
    else if (option == "--fixed-controller-mask")
      result.fixed_controller_mask = parse_controller_mask(value);
    else
      throw std::invalid_argument("unknown option: " + std::string(option));
  }
  if (result.pack.empty())
    throw std::invalid_argument("--content-pack is required; use `project.py frontend run` for first-launch extraction");
  return result;
}

struct RuntimeContent {
  explicit RuntimeContent(const std::filesystem::path &path) : pack(path) {}
  unirally::ClassicContentPack pack;

  unirally::MovementContent movement() {
    return {{entry("physics.track.dragster.data"),
             entry("physics.rider.collision-poses"),
             entry("physics.rider.collision-templates")},
            {entry("physics.track.dragster.tile-columns"),
             entry("physics.track.dragster.tile-flags")},
            entry("physics.track.progress-transitions"),
            entry("physics.rider.pose-slopes"),
            entry("physics.rider.displacement-table"),
            entry("physics.rider.idle-pose-table"),
            entry("physics.reward.rotation-value"),
            entry("physics.reward.rotation-class"),
            {entry("physics.speed.masks"),
             entry("physics.speed.decrements")}};
  }

  unirally::PresentationContent presentation() {
    return {entry("physics.track.dragster.data"),
            entry("presentation.track.dragster.bg1-tiles.v1"),
            entry("presentation.track.dragster.bg2-tiles.v1"),
            entry("presentation.track.dragster.bg2-map.v1"),
            entry("presentation.classic.palette.v1"),
            entry("presentation.classic.font.v1"),
            entry("presentation.rider.mike.race-tiles.v1"),
            entry("presentation.result.classic.font-layout.v1"),
            entry("presentation.effect.go-window.v1"),
            entry("presentation.effect.winner-window.v1"),
            entry("presentation.result.classic.base-vram.v1"),
            entry("presentation.result.classic.palette.v1"),
            entry("presentation.result.classic.palette-tail.v1")};
  }

private:
  std::span<const std::uint8_t> entry(const char *id) { return pack.entry(id); }
};

void draw(SDL_Renderer *renderer, SDL_Texture *texture,
          const unirally::RgbFrame &frame) {
  if (!SDL_UpdateTexture(texture, nullptr, frame.pixels.data(),
                         static_cast<int>(unirally::RgbFrame::width * 3)))
    throw sdl_error("cannot update presentation texture");
  int width{}, height{};
  if (!SDL_GetCurrentRenderOutputSize(renderer, &width, &height))
    throw sdl_error("cannot query renderer output size");
  const auto view = unirally::app::integer_viewport(width, height);
  if (!SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) ||
      !SDL_RenderClear(renderer))
    throw sdl_error("cannot clear renderer");
  const SDL_FRect destination{static_cast<float>(view.x),
                              static_cast<float>(view.y),
                              static_cast<float>(view.width),
                              static_cast<float>(view.height)};
  if (!SDL_RenderTexture(renderer, texture, nullptr, &destination))
    throw sdl_error("cannot draw presentation texture");
  if (!SDL_RenderPresent(renderer))
    throw sdl_error("cannot present rendered frame");
}
} // namespace

int main(int argc, char **argv) try {
  const auto parsed = options(argc, argv);
  if (!parsed)
    return 0;
  RuntimeContent content(parsed->pack); // validate before SDL or gameplay
  auto movement_content = content.movement();
  auto presentation_content = content.presentation();
  auto dragster_state=unirally::classic_crawler_dragster_start();
  const auto zoom_content=parsed->zoom_zoo?unirally::zoom_zoo_content(content.pack):unirally::ZoomZooContent{};
  auto zoom_state=parsed->zoom_zoo?unirally::classic_crawler_zoom_zoo_start(zoom_content):unirally::ZoomZooState{};
  auto& state=parsed->zoom_zoo?zoom_state.movement:dragster_state;
  auto zoom_hud_state=zoom_state; // State before the latest update, for the HUD.
  unsigned restarts=0;


  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    throw sdl_error("SDL initialization failed");
  SdlQuitter quit;
  const auto flags = SDL_WINDOW_RESIZABLE |
                     (parsed->hidden ? SDL_WINDOW_HIDDEN : 0U);
  Window window(SDL_CreateWindow(parsed->zoom_zoo?"Unirally — Classic CRAWLER / ZOOM ZOO":"Unirally — Classic CRAWLER / DRAGSTER",
                                 768, 672, flags));
  if (!window)
    throw sdl_error("window creation failed");
  if (!SDL_SetWindowMinimumSize(window.get(), 256, 224))
    throw sdl_error("cannot set minimum window size");
  Renderer renderer(SDL_CreateRenderer(window.get(), nullptr));
  if (!renderer)
    throw sdl_error("renderer creation failed");
  Texture texture(SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_RGB24,
                                    SDL_TEXTUREACCESS_STREAMING, 256, 224));
  if (!texture)
    throw sdl_error("texture creation failed");
  if (!SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST))
    throw sdl_error("cannot select nearest-neighbor scaling");

  std::cout << "Classic pack validated: " << parsed->pack << '\n'
            << "PAL scheduler: 50 Hz, maximum catch-up 4 updates\n"
            << "Audio is intentionally not implemented in M3.\n";

  unirally::app::InputState input;
  Gamepads gamepads(input);
  unirally::app::PalScheduler scheduler(SDL_GetTicksNS());
  bool running = true, redraw = true;
  std::uint32_t updates{};
  std::uint32_t rendered_frames{}, pose_fallback_frames{};
  std::uint32_t identical_redraws{}, current_identical_run{},
      longest_identical_run{};
  std::uint32_t identical_fallback_race_redraws{},
      current_identical_fallback_race_run{},
      longest_identical_fallback_race_run{};
  std::optional<unirally::RgbFrame> previous_frame;
  std::uint32_t mapped_key_down_events{}, mapped_key_up_events{};
  // Opponent trick telemetry. The multi-axis branch is what aborted before it
  // was recovered, and it is invisible in the other counters, so a live run
  // can otherwise only show the absence of a crash rather than the presence of
  // the repaired path. seen_selectors is a bitmask over selector values 0-7.
  std::uint32_t opponent_trick_updates{}, opponent_multi_axis_updates{}, seen_selectors{};
  std::uint32_t nonzero_input_updates{}, simultaneous_input_updates{};
  std::uint32_t neutral_updates_after_input{}, focus_loss_events{};
  std::uint32_t focus_loss_nonzero_clears{};
  bool observed_nonzero_input{};
  std::array<std::uint16_t, 2> last_ports{};
  auto position = unirally::app::presentation_position(state.riders[0].motion.x);
  unirally::app::LivePresentation live_presentation;
  bool reported_held_frame{};
  while (running) {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT: running = false; break;
      case SDL_EVENT_WINDOW_FOCUS_LOST:
        ++focus_loss_events;
        if (input.snapshot() != std::array<std::uint16_t, 2>{})
          ++focus_loss_nonzero_clears;
        input.clear();
        scheduler.pause(SDL_GetTicksNS());
        break;
      case SDL_EVENT_WINDOW_FOCUS_GAINED:
        scheduler.resume(SDL_GetTicksNS());
        break;
      case SDL_EVENT_WINDOW_EXPOSED:
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: redraw = true; break;
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP:
        if(parsed->zoom_zoo && event.type==SDL_EVENT_KEY_DOWN && !event.key.repeat &&
           event.key.scancode==SDL_SCANCODE_RETURN && zoom_state.result_updates==115) {
          unirally::restart_zoom_zoo(zoom_state,zoom_content);zoom_hud_state=zoom_state;
          input.clear();live_presentation=unirally::app::LivePresentation{};++restarts;redraw=true;
          break;
        }
        if (const auto key = keyboard_key(event.key.scancode)) {
          input.keyboard(*key, event.type == SDL_EVENT_KEY_DOWN);
          if (event.type == SDL_EVENT_KEY_DOWN)
            ++mapped_key_down_events;
          else
            ++mapped_key_up_events;
        }
        break;
      case SDL_EVENT_GAMEPAD_ADDED: gamepads.added(event.gdevice.which); break;
      case SDL_EVENT_GAMEPAD_REMOVED: gamepads.removed(event.gdevice.which); break;
      case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
      case SDL_EVENT_GAMEPAD_BUTTON_UP:
        gamepads.button(event.gbutton.which, event.gbutton.button,
                        event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
        break;
      default: break;
      }
    }

    const auto now = SDL_GetTicksNS();
    const auto due = scheduler.updates_due(now);
    for (std::uint32_t index = 0; index < due; ++index) {
      auto ports = input.snapshot();
      if (parsed->fixed_controller_mask.has_value())
        ports[0] = *parsed->fixed_controller_mask;
      last_ports = ports;
      if (ports[0] != 0) {
        ++nonzero_input_updates;
        observed_nonzero_input = true;
        if ((ports[0] & static_cast<std::uint16_t>(ports[0] - 1U)) != 0)
          ++simultaneous_input_updates;
      } else if (observed_nonzero_input) {
        ++neutral_updates_after_input;
      }
      if(parsed->zoom_zoo) {
        const auto previous_simulation_frame=zoom_state.movement.frame;
        zoom_hud_state=zoom_state;
        const auto buttons=unirally::app::controller_buttons(ports[0]);
        if(zoom_state.result_updates==115 && buttons.start)
          unirally::restart_zoom_zoo(zoom_state,zoom_content);
        else unirally::update_zoom_zoo(zoom_state,buttons,zoom_content);
        // Selector 0 is a real trick (the flat path's negative-velocity
        // rotation), so the impulse is the activity signal; the selector alone
        // would silently drop it.
        if(zoom_state.movement.opponent_ai.impulse_countdown) {
          const auto selector=zoom_state.movement.opponent_ai.trick_selector;
          ++opponent_trick_updates;
          if(selector&6U)++opponent_multi_axis_updates;
          if(selector<8U)seen_selectors|=1U<<selector;
        }
        if(zoom_state.movement.frame<previous_simulation_frame) {
          // Both keyboard and gamepad navigation replace all simulation/art
          // state. A physically held Start cannot immediately pause the new race.
          input.clear();live_presentation=unirally::app::LivePresentation{};++restarts;
          zoom_hud_state=zoom_state;
        }
      }
      else unirally::update_movement(state,
                                unirally::app::controller_buttons(ports[0]),
                                movement_content);
      ++updates;
      if (state.finish.phase == unirally::RacePhase::Racing)
        position = unirally::app::presentation_position(state.riders[0].motion.x);
      redraw = true;
      if (parsed->maximum_updates != 0 && updates >= parsed->maximum_updates) {
        running = false;
        break;
      }
    }
    if (redraw) {
      const auto canonical_before = parsed->zoom_zoo?unirally::serialize_zoom_zoo(zoom_state):unirally::serialize_movement_state(state);
      const auto live_frame =
          parsed->zoom_zoo?live_presentation.render_zoom(zoom_state,zoom_hud_state,content.pack):live_presentation.render(state, position, presentation_content);
      if (live_frame.used_pose_fallback && !reported_held_frame) {
        std::cout << "Presentation note: unsupported intermediate rider poses use the last recovered rider art while the scene stays current.\n";
        reported_held_frame = true;
      }
      ++rendered_frames;
      if (live_frame.used_pose_fallback)
        ++pose_fallback_frames;
      if (previous_frame && previous_frame->pixels == live_frame.frame.pixels) {
        ++identical_redraws;
        ++current_identical_run;
        longest_identical_run =
            std::max(longest_identical_run, current_identical_run);
      } else {
        current_identical_run = 0;
      }
      if (previous_frame && previous_frame->pixels == live_frame.frame.pixels &&
          live_frame.used_pose_fallback &&
          state.finish.phase == unirally::RacePhase::Racing) {
        ++identical_fallback_race_redraws;
        ++current_identical_fallback_race_run;
        longest_identical_fallback_race_run = std::max(
            longest_identical_fallback_race_run,
            current_identical_fallback_race_run);
      } else {
        current_identical_fallback_race_run = 0;
      }
      if ((parsed->zoom_zoo?unirally::serialize_zoom_zoo(zoom_state):unirally::serialize_movement_state(state)) != canonical_before)
        throw std::logic_error("presentation mutated canonical gameplay state");
      draw(renderer.get(), texture.get(), live_frame.frame);
      previous_frame = live_frame.frame;
      redraw = false;
    }
    if (running)
      SDL_Delay(1);
  }
  std::cout << "Presentation frames: " << rendered_frames
            << "; rider-pose fallback frames: " << pose_fallback_frames
            << "; identical consecutive redraws: " << identical_redraws
            << "; longest identical run: " << longest_identical_run
            << "; identical fallback race redraws: "
            << identical_fallback_race_redraws
            << "; longest identical fallback race run: "
            << longest_identical_fallback_race_run
            << '\n'
            << "Live input: mapped key down/up " << mapped_key_down_events
            << '/' << mapped_key_up_events << "; nonzero updates "
            << nonzero_input_updates << "; simultaneous updates "
            << simultaneous_input_updates << "; neutral updates after input "
            << neutral_updates_after_input << "; focus losses/active clears "
            << focus_loss_events << '/' << focus_loss_nonzero_clears << '\n'
            << "Final native state: updates " << updates << "; frame "
            << state.frame << "; controller-0 mask " << last_ports[0]
            << "; player x " << state.riders[0].motion.x << "; velocity x "
            << state.riders[0].motion.velocity_x << '\n';
  if(!parsed->zoom_zoo)std::cout<<"DRAGSTER race phase "<<static_cast<unsigned>(state.finish.phase)
      <<"; outcome "<<static_cast<unsigned>(state.finish.outcome)<<'\n';
  if(parsed->zoom_zoo)std::cout<<"ZOOM ZOO result updates "<<zoom_state.result_updates<<"; restarts "<<restarts
      <<"; totals "<<zoom_state.race.total_times[0]<<'/'<<zoom_state.race.total_times[1]<<'\n';
  if(parsed->zoom_zoo) {
    std::cout<<"Opponent tricks: updates "<<opponent_trick_updates<<"; multi-axis updates "
             <<opponent_multi_axis_updates<<"; selectors seen";
    if(!seen_selectors)std::cout<<" none";
    else for(unsigned s=0;s<8;++s)if(seen_selectors&(1U<<s))std::cout<<' '<<s;
    std::cout<<'\n';
  }
  return 0;
} catch (const std::exception &error) {
  std::cerr << "Unirally launch failed: " << error.what() << '\n';
  return 1;
}
