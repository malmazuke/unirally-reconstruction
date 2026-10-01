// AUDIO-FIRST-RACE laboratory runner: the exact native title/menu work until a
// 1P menu exit, then each frame's reported queue operations at frame-anchored
// clocks (D-0010). Inputs are a validated pack, the controller schedule and a
// cue file; no original clock, event or PCM is an input.
#include "audio_title_menu_data.hpp"
#include <iostream>
#include <map>

namespace {
class Controllers final : public unirally::AudioControllerSource {
public:
  std::vector<std::array<std::uint16_t, 2>> rows;
  explicit Controllers(const char *path) {
    std::ifstream file(path);
    unsigned frame, first, second;
    if (!file)
      throw std::runtime_error("cannot read controller schedule");
    while (file >> frame >> first >> second) {
      if (frame != rows.size() || first > 65535 || second > 65535)
        throw std::invalid_argument("invalid native controller schedule");
      rows.push_back({static_cast<std::uint16_t>(first),
                      static_cast<std::uint16_t>(second)});
    }
    if (!file.eof())
      throw std::invalid_argument("controller schedule truncated");
  }
  std::uint16_t controller_word(std::uint64_t ticks, unsigned port) override {
    const auto frame = ticks / 425568 + 1;
    return frame < rows.size() ? rows[frame][port] : 0;
  }
};
class Events final : public unirally::AudioEngineEventSink {
public:
  std::ofstream file;
  explicit Events(const char *path) : file(path) {
    if (!file)
      throw std::runtime_error("cannot write native events");
  }
  void event(char kind, std::uint64_t smp, std::uint64_t cpu,
             std::uint16_t address, std::uint8_t value) override {
    file << kind << ' ' << smp << ' ' << cpu << ' ' << address << ' '
         << unsigned(value) << '\n';
  }
};
// "FRAME E COMMAND PARAMETER", "FRAME D early|late|wait", "FRAME L race|title".
std::map<std::uint32_t, unirally::AudioCueList> read_cues(const char *path) {
  std::ifstream file(path);
  if (!file)
    throw std::runtime_error("cannot read cue file");
  std::map<std::uint32_t, unirally::AudioCueList> cues;
  std::uint32_t frame;
  std::string kind;
  while (file >> frame >> kind) {
    auto &list = cues[frame];
    if (kind == "E") {
      unsigned command, parameter;
      if (!(file >> command >> parameter) || command > 63 || parameter > 255)
        throw std::invalid_argument("invalid enqueue cue");
      list.push_back(
          unirally::audio_enqueue(static_cast<std::uint8_t>(command),
                                  static_cast<std::uint8_t>(parameter)));
    } else if (kind == "D" || kind == "L") {
      std::string what;
      file >> what;
      if (kind == "D")
        list.push_back(unirally::audio_dispatch(
            what == "early"       ? unirally::AudioDispatchSite::race_early
            : what == "late"      ? unirally::AudioDispatchSite::race_late
            : what == "countdown" ? unirally::AudioDispatchSite::countdown
            : what == "finish"    ? unirally::AudioDispatchSite::finish_fade
            : what == "choice"    ? unirally::AudioDispatchSite::race_choice
                                  : unirally::AudioDispatchSite::frame_wait));
      else
        list.push_back(unirally::audio_load(
            what == "race" ? unirally::AudioSessionLoad::first_race
                           : unirally::AudioSessionLoad::title_return));
    } else
      throw std::invalid_argument("unknown cue kind");
  }
  return cues;
}
} // namespace
int main(int argc, char **argv) {
  try {
    if (argc != 8)
      throw std::invalid_argument(
          "race_audio_runner PACK INPUT CUES EVENTS PCM LAST_FRAME DSP_END");
    const auto content = audio_test::content(argv[1]);
    if (!content.has_race_set())
      throw std::invalid_argument("pack lacks the race sound set");
    Controllers controllers(argv[2]);
    const auto cues = read_cues(argv[3]);
    Events events(argv[4]);
    std::ofstream pcm(argv[5], std::ios::binary);
    if (!pcm)
      throw std::runtime_error("cannot write PCM");
    unirally::NativeTitleMenuAudio audio(content, controllers, &events);
    audio.initialize_title();
    while (audio.phase() == unirally::TitleMenuAudioPhase::title_hold)
      audio.title_frame();
    audio.reveal_menu();
    auto action = unirally::AudioCpuMenuAction::waiting;
    while (action == unirally::AudioCpuMenuAction::waiting)
      action = audio.menu_frame();
    const auto exit_clock = audio.cpu_ticks();
    const auto exit_frame =
        static_cast<std::uint32_t>((exit_clock - 306900) / 425568 + 1);
    std::cout << "menu_exit_action=" << unsigned(action)
              << " cpu_clock=" << exit_clock << " frame=" << exit_frame << '\n';
    const auto last = static_cast<std::uint32_t>(std::stoul(argv[6]));
    for (auto frame = exit_frame; frame <= last; ++frame) {
      const auto found = cues.find(frame);
      audio.cue_frame(frame, found == cues.end() ? unirally::AudioCueList{}
                                                 : found->second);
    }
    audio.finish_pcm_to(std::stoull(argv[7]));
    audio_test::write_pcm(pcm, audio.take_pcm());
    std::cout << "computed_cpu_clock=" << audio.cpu_ticks() << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
