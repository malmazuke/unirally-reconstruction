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
#include <fstream>
#include <iostream>
#include <map>
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

  bool added(SDL_JoystickID id) {
    if (port_for(id))
      return false;
    const auto empty = std::find_if(slots_.begin(), slots_.end(),
                                    [](const auto &slot) { return !slot; });
    if (empty == slots_.end())
      return false;
    Gamepad opened(SDL_OpenGamepad(id));
    if (!opened)
      throw sdl_error("cannot open gamepad");
    const auto port = static_cast<std::size_t>(empty - slots_.begin());
    *empty = OpenGamepad{id, std::move(opened)};
    std::cout << "Gamepad connected to controller port " << port << ": "
              << SDL_GetGamepadName(empty->value().handle.get()) << '\n';
    return true;
  }

  // Returns whether the removal cleared a held button.
  bool removed(SDL_JoystickID id) {
    const auto port = port_for(id);
    if (!port)
      return false;
    const bool held = input_.gamepad_mask(*port) != 0;
    input_.disconnect(*port);
    slots_[*port].reset();
    std::cout << "Gamepad removed from controller port " << unsigned(*port)
              << '\n';
    return held;
  }

  // The port a mapped button reached, if any.
  std::optional<std::uint8_t> button(SDL_JoystickID id, Uint8 button, bool pressed) {
    const auto port = port_for(id);
    const auto mapped = gamepad_button(button);
    if (!port || !mapped)
      return std::nullopt;
    input_.gamepad(*port, *mapped, pressed);
    return port;
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
  unirally::ClassicRaceTrack track{unirally::ClassicRaceTrack::Dragster};
  bool track_given{}; // without --track the app starts at power-on (the front end)
  // The front end's pads by its frame, from a laboratory input script (smoke-test aid).
  std::map<std::uint32_t, unirally::FrontEndPads> front_end_inputs;
};

// "frame pad1 pad2" rows, the pads as hex SNES words, as `front_end_runner` reads them.
std::map<std::uint32_t, unirally::FrontEndPads> read_front_end_inputs(const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in) throw std::invalid_argument("cannot open --front-end-inputs");
  std::map<std::uint32_t, unirally::FrontEndPads> rows;
  std::uint32_t frame{};
  std::string one, two;
  while (in >> frame >> one >> two)
    rows[frame] = {static_cast<std::uint16_t>(std::stoul(one, nullptr, 16)),
                   static_cast<std::uint16_t>(std::stoul(two, nullptr, 16))};
  return rows;
}

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
      << "Usage: unirally --content-pack PATH [--track dragster|zoom-zoo|NN] [--updates N] [--hidden]\n"
      << "       NN: a race track's number (its index in the ROM) with a recovered scenario\n"
      << "       unirally --supported-profiles   (print the pack profiles this build reads)\n"
      << "Without --track it starts at power-on: the Nintendo screen, the title and the main menu;\n"
      << "1P leads to the one-player screens and the race chosen there. With --track it starts in\n"
      << "that race. PAL 50 Hz.\n"
      << "Keyboard: arrows, Z=B, X=Y, A=A, S=X, Q=L, W=R, Enter=Start.\n"
      << "Gamepad: D-pad, South=B, West=Y, East=A, North=X, shoulders=L/R, Start, Back=Select;\n"
      << "the analog stick is not mapped. Two gamepads are tracked; this slice consumes port 0 only.\n"
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
    if (option == "--supported-profiles") {
      for (const auto profile : unirally::supported_pack_profiles())
        std::cout << profile << '\n';
      return std::nullopt;
    }
    if (index + 1 >= argc)
      throw std::invalid_argument(std::string(option) + " requires a value");
    const std::string_view value(argv[++index]);
    if (option == "--track") {
      result.track_given = true;
      if(value=="dragster")result.track=unirally::ClassicRaceTrack::Dragster;
      else if(value=="zoom-zoo")result.track=unirally::ClassicRaceTrack::ZoomZoo;
      else {
        unsigned track_index{};
        const auto parsed_index=std::from_chars(value.data(),value.data()+value.size(),track_index);
        if(parsed_index.ec!=std::errc{} || parsed_index.ptr!=value.data()+value.size() || track_index>44U ||
           !unirally::classic_race_has_scenario(unirally::ClassicRaceTrack{static_cast<std::uint8_t>(track_index)}))
          throw std::invalid_argument("unknown track: use dragster, zoom-zoo or the number of a race track with a recovered scenario");
        result.track=unirally::ClassicRaceTrack{static_cast<std::uint8_t>(track_index)};
      }
    } else if (option == "--content-pack")
      result.pack = value;
    else if (option == "--updates")
      result.maximum_updates = parse_updates(value);
    else if (option == "--fixed-controller-mask")
      result.fixed_controller_mask = parse_controller_mask(value);
    else if (option == "--front-end-inputs")
      result.front_end_inputs = read_front_end_inputs(value);
    else
      throw std::invalid_argument("unknown option: " + std::string(option));
  }
  if (result.pack.empty())
    throw std::invalid_argument("--content-pack is required; use `project.py frontend run` for first-launch extraction");
  return result;
}

std::string window_title(const std::string& track_name) {
  return "Unirally \u2014 Classic / "+track_name;
}

struct RuntimeContent {
  explicit RuntimeContent(const std::filesystem::path &path) : pack(path) {}
  unirally::ClassicContentPack pack;
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
  auto track=parsed->track; // NOW PLAYING can choose another race
  const bool zoom_zoo=track==unirally::ClassicRaceTrack::ZoomZoo;
  // Both tracks run the shared race engine (R-0038) and are drawn by the shared
  // renderer from their own track content. DRAGSTER's trick, landing, reversal
  // and finish tables are track-independent ROM tables that only the two-track
  // pack carries; the 25-entry DRAGSTER pack cannot play them.
  if(!zoom_zoo && content.pack.optional_entry("zoom.landing-response-matrices").empty())
    throw std::invalid_argument("DRAGSTER and the other tracks need the full content pack for jumps, brakes, reversal and tricks; "
                                "create it from your ROM with: python3 tools/project.py frontend run --track dragster "
                                "--pack local/classic-pal-crawler-tracks-v19.pack --rom PATH");
  auto zoom_content=unirally::classic_race_content(content.pack,track);
  auto race_presentation=unirally::classic_race_presentation_content(content.pack,track);
  auto zoom_state=unirally::classic_race_start(zoom_content,unirally::classic_race_scenario(track));
  auto& state=zoom_state.movement;
  auto zoom_hud_state=zoom_state; // State before the latest update, for the HUD.
  unsigned restarts=0;
  // A restart from the stable result proves a completed race; one from the pause menu does not.
  unsigned results_reached=0,result_restarts=0,pause_restarts=0;


  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    throw sdl_error("SDL initialization failed");
  SdlQuitter quit;
  const auto flags = SDL_WINDOW_RESIZABLE |
                     (parsed->hidden ? SDL_WINDOW_HIDDEN : 0U);
  Window window(SDL_CreateWindow(window_title(race_presentation.track_name).c_str(),
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
  // Gamepad witnesses, separate from the keyboard so a live run can show which drove it.
  std::uint32_t gamepad_connections{}, gamepad_button_down_events{}, gamepad_button_up_events{},
      gamepad_buttons_pressed{}, gamepad_nonzero_updates{}, gamepad_only_updates{},
      gamepad_pause_openings{}, gamepad_only_restarts{}, gamepad_removals{}, gamepad_removal_active_clears{};
  std::uint32_t nonzero_input_updates{}, simultaneous_input_updates{};
  std::uint32_t neutral_updates_after_input{}, focus_loss_events{};
  std::uint32_t focus_loss_nonzero_clears{};
  bool observed_nonzero_input{};
  std::array<std::uint16_t, 2> last_ports{};
  unirally::app::LivePresentation live_presentation;
  bool reported_held_frame{};
  // Without --track the session starts at power-on; NOW PLAYING's Race starts the race.
  std::optional<unirally::app::FrontEndSession> front_end;
  if (!parsed->track_given) front_end.emplace(content.pack);
  // During a race the front end waits here for the race's result load.
  std::optional<unirally::app::FrontEndSession> waiting_front_end;
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
        if(event.type==SDL_EVENT_KEY_DOWN && !event.key.repeat &&
           event.key.scancode==SDL_SCANCODE_RETURN && zoom_state.result_updates &&
           zoom_state.result_updates==unirally::stable_result_updates(zoom_state)) {
          unirally::restart_zoom_zoo(zoom_state,zoom_content);zoom_hud_state=zoom_state;
          input.clear();live_presentation=unirally::app::LivePresentation{};++restarts;++result_restarts;redraw=true;
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
      case SDL_EVENT_GAMEPAD_ADDED:
        if (gamepads.added(event.gdevice.which))
          ++gamepad_connections;
        break;
      case SDL_EVENT_GAMEPAD_REMOVED:
        ++gamepad_removals;
        if (gamepads.removed(event.gdevice.which))
          ++gamepad_removal_active_clears;
        break;
      case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
      case SDL_EVENT_GAMEPAD_BUTTON_UP: {
        const bool down = event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
        if (gamepads.button(event.gbutton.which, event.gbutton.button, down) == std::uint8_t{0}) {
          if (down) {
            ++gamepad_button_down_events;
            if (event.gbutton.button < 32)
              gamepad_buttons_pressed |= 1U << event.gbutton.button;
          } else {
            ++gamepad_button_up_events;
          }
        }
        break;
      }
      default: break;
      }
    }

    const auto now = SDL_GetTicksNS();
    const auto due = scheduler.updates_due(now);
    for (std::uint32_t index = 0; index < due; ++index) {
      auto ports = input.snapshot();
      const auto gamepad_mask = input.gamepad_mask(0);
      const bool gamepad_only = gamepad_mask != 0 && input.keyboard_mask() == 0;
      if (gamepad_mask != 0)
        ++gamepad_nonzero_updates;
      if (gamepad_only)
        ++gamepad_only_updates;
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
      bool race_chosen = false;
      if (front_end && !parsed->front_end_inputs.empty()) {
        const auto row = parsed->front_end_inputs.find(front_end->front_end_frame());
        race_chosen = front_end->update(row == parsed->front_end_inputs.end()
                                            ? unirally::FrontEndPads{}
                                            : row->second);
      } else if (front_end) {
        race_chosen = front_end->update(ports);
      }
      if (front_end) {
        if (race_chosen) {
          // The race NOW PLAYING chose, if it is not the one the app started with.
          const auto chosen=front_end->race_track();
          std::cout << "Front end: race " << unsigned(chosen.index) << " chosen after " << front_end->frames()
                    << " frames (front-end frame " << front_end->front_end_frame() << ")\n";
          if(!(chosen==track)) {
            track=chosen;
            zoom_content=unirally::classic_race_content(content.pack,chosen);
            race_presentation=unirally::classic_race_presentation_content(content.pack,chosen);
            SDL_SetWindowTitle(window.get(),window_title(race_presentation.track_name).c_str());
          }
          // A fresh race each time: after a result NOW PLAYING can choose the same track again.
          zoom_state=unirally::classic_race_start(zoom_content,unirally::classic_race_scenario(chosen));
          zoom_hud_state=zoom_state;
          live_presentation=unirally::app::LivePresentation{};
          waiting_front_end=std::move(front_end);
          front_end.reset();
          input.clear();
        }
      } else {
        const auto previous_simulation_frame=zoom_state.movement.frame;
        const bool was_paused=zoom_state.pause.selection!=0;
        const bool at_stable_result=zoom_state.result_updates!=0 &&
            zoom_state.result_updates==unirally::stable_result_updates(zoom_state);
        zoom_hud_state=zoom_state;
        // A keyboard and an analog stick can report opposing directions that a
        // SNES pad's rocker cannot; update_zoom_zoo drops them for both tracks.
        const auto buttons=unirally::app::controller_buttons(ports[0]);
        if(at_stable_result && buttons.start)
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
        if(!was_paused && zoom_state.pause.selection &&
           (gamepad_mask&unirally::app::button_mask(unirally::app::LogicalButton::Start)))++gamepad_pause_openings;
        if(!at_stable_result && zoom_state.result_updates &&
           zoom_state.result_updates==unirally::stable_result_updates(zoom_state))++results_reached;
        if(zoom_state.movement.frame<previous_simulation_frame) {
          if(gamepad_only)++gamepad_only_restarts;
          if(at_stable_result)++result_restarts;
          else ++pause_restarts;
          // Both keyboard and gamepad navigation replace all simulation/art
          // state. A physically held Start cannot immediately pause the new race.
          input.clear();live_presentation=unirally::app::LivePresentation{};++restarts;
          zoom_hud_state=zoom_state;
        } else {
          live_presentation.observe_update(zoom_hud_state,zoom_state,content.pack);
        }
        // The race's result load: the menus' result screen takes over (R-0057, R-0058).
        if(waiting_front_end && zoom_state.result_updates==1) {
          waiting_front_end->return_from_race(zoom_state);
          front_end=std::move(waiting_front_end);
          waiting_front_end.reset();
          // The input is kept: a button held from the race holds a one-run result (`$80:C24C`)
          // and ends a lap result at its first test (`$80:B6D3`), as in the original.
          std::cout<<"Front end: race returned at front-end frame "<<front_end->front_end_frame()
                   <<"; totals "<<zoom_state.race.total_times[0]<<'/'<<zoom_state.race.total_times[1]<<'\n';
        }
      }
      ++updates;
      redraw = true;
      if (parsed->maximum_updates != 0 && updates >= parsed->maximum_updates) {
        running = false;
        break;
      }
    }
    if (redraw && front_end) {
      draw(renderer.get(), texture.get(), front_end->frame());
      redraw = false;
    }
    if (redraw) {
      const auto canonical_before = unirally::serialize_zoom_zoo(zoom_state);
      const auto live_frame =
          live_presentation.render_race(zoom_state,zoom_hud_state,race_presentation);
      if (live_frame.used_pose_fallback && !reported_held_frame) {
        std::cout << "Presentation note: a rider pose outside the packed tables holds that rider's last drawn pose.\n";
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
          !zoom_state.race.riders[0].finished) {
        ++identical_fallback_race_redraws;
        ++current_identical_fallback_race_run;
        longest_identical_fallback_race_run = std::max(
            longest_identical_fallback_race_run,
            current_identical_fallback_race_run);
      } else {
        current_identical_fallback_race_run = 0;
      }
      if (unirally::serialize_zoom_zoo(zoom_state) != canonical_before)
        throw std::logic_error("presentation mutated canonical gameplay state");
      draw(renderer.get(), texture.get(), live_frame.frame);
      previous_frame = live_frame.frame;
      redraw = false;
    }
    if (running)
      SDL_Delay(1);
  }
  if (front_end)
    std::cout << "Front end: frames " << front_end->frames() << "; notices "
              << front_end->notices() << "; returns to the main menu "
              << front_end->returns_to_menu() << "; races returned " << front_end->races()
              << "; in the menus at the end\n";
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
  if(!(track==unirally::ClassicRaceTrack::ZoomZoo)) {
    const auto shown=unirally::classic_finish_view(zoom_state);
    std::cout<<race_presentation.track_name<<" race phase "<<static_cast<unsigned>(shown.phase)
      <<"; outcome "<<static_cast<unsigned>(shown.outcome)<<'\n';
  }
  std::cout<<race_presentation.track_name<<" result updates "<<zoom_state.result_updates<<"; restarts "<<restarts
      <<"; totals "<<zoom_state.race.total_times[0]<<'/'<<zoom_state.race.total_times[1]
      <<"; stable results reached "<<results_reached<<"; restarts from result/pause "
      <<result_restarts<<'/'<<pause_restarts<<'\n';
  if(track==unirally::ClassicRaceTrack::ZoomZoo) {
    std::cout<<"Opponent tricks: updates "<<opponent_trick_updates<<"; multi-axis updates "
             <<opponent_multi_axis_updates<<"; selectors seen";
    if(!seen_selectors)std::cout<<" none";
    else for(unsigned s=0;s<8;++s)if(seen_selectors&(1U<<s))std::cout<<' '<<s;
    std::cout<<'\n';
  }
  std::cout << "Gamepad input: connections " << gamepad_connections
            << "; port-0 button down/up " << gamepad_button_down_events << '/'
            << gamepad_button_up_events << "; buttons pressed";
  if (!gamepad_buttons_pressed)
    std::cout << " none";
  for (unsigned button = 0; button < 32; ++button)
    if (gamepad_buttons_pressed & (1U << button))
      std::cout << ' ' << SDL_GetGamepadStringForButton(static_cast<SDL_GamepadButton>(button));
  std::cout << "; nonzero updates " << gamepad_nonzero_updates
            << "; gamepad-only nonzero updates " << gamepad_only_updates
            << "; pause openings " << gamepad_pause_openings
            << "; gamepad-only restarts " << gamepad_only_restarts
            << "; removals/active clears " << gamepad_removals << '/'
            << gamepad_removal_active_clears << '\n';
  return 0;
} catch (const std::exception &error) {
  std::cerr << "Unirally launch failed: " << error.what() << '\n';
  return 1;
}
