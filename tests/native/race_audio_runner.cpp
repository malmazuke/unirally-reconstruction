// AUDIO-FIRST-RACE laboratory runner: the exact native title/menu work until a
// 1P menu exit, then each frame's reported queue operations at frame-anchored
// clocks (D-0010). Inputs are a validated pack, the controller schedule and a
// cue file; no original clock, event or PCM is an input.
#include "audio_title_menu_data.hpp"
#include "title_menu_audio_playback.hpp"
#include <iostream>
#include <map>
#include <memory>

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
// "FRAME E COMMAND PARAMETER", "FRAME D early|late|...|wait", "FRAME L
// race|title", and the app's unresolved "FRAME R RIDER ROTATING", which the
// audio side's latches resolve.
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
    } else if (kind == "R") {
      unsigned rider, rotating;
      if (!(file >> rider >> rotating) || rider > 1 || rotating > 1)
        throw std::invalid_argument("invalid rotation cue");
      list.push_back(unirally::audio_rotation(rider, rotating != 0));
    } else if (kind == "D" || kind == "L") {
      std::string what;
      file >> what;
      if (kind == "D")
        list.push_back(unirally::audio_dispatch(
            what == "early"        ? unirally::AudioDispatchSite::race_early
            : what == "late"       ? unirally::AudioDispatchSite::race_late
            : what == "countdown"  ? unirally::AudioDispatchSite::countdown
            : what == "finish"     ? unirally::AudioDispatchSite::finish_fade
            : what == "choice"     ? unirally::AudioDispatchSite::race_choice
            : what == "pause"      ? unirally::AudioDispatchSite::pause
            : what == "pause-fade" ? unirally::AudioDispatchSite::pause_fade
            : what == "pause-continue"
                ? unirally::AudioDispatchSite::pause_continue
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
    if (argc != 8 && argc != 12)
      throw std::invalid_argument(
          "race_audio_runner PACK INPUT CUES EVENTS PCM LAST_FRAME DSP_END "
          "[OUTPUT_RATE SAVE_FRAME STATE RESTORE] (save 0: none; restore: - or "
          "file)");
    const auto content = audio_test::content(argv[1]);
    if (!content.has_race_set())
      throw std::invalid_argument("pack lacks the race sound set");
    Controllers controllers(argv[2]);
    const auto cues = read_cues(argv[3]);
    Events events(argv[4]);
    std::ofstream pcm(argv[5], std::ios::binary);
    if (!pcm)
      throw std::runtime_error("cannot write PCM");
    const auto last = static_cast<std::uint32_t>(std::stoul(argv[6]));
    // Raw mode writes the DSP's pairs; playback mode resamples to OUTPUT_RATE,
    // drains 257 output pairs after each frame and can save after a cued frame
    // or continue a saved run in a fresh process.
    std::unique_ptr<unirally::NativeTitleMenuAudioPlayback> playback;
    std::unique_ptr<unirally::NativeTitleMenuAudio> raw;
    if (argc == 12)
      playback = std::make_unique<unirally::NativeTitleMenuAudioPlayback>(
          content, controllers, static_cast<std::uint32_t>(std::stoul(argv[8])),
          &events);
    else
      raw = std::make_unique<unirally::NativeTitleMenuAudio>(
          content, controllers, &events);
    auto &audio = playback ? playback->native() : *raw;
    const auto save_frame =
        argc == 12 ? static_cast<std::uint32_t>(std::stoul(argv[9])) : 0U;
    const auto drain = [&] {
      if (playback)
        audio_test::write_pcm(pcm, playback->take_pairs(257));
    };
    std::uint32_t first = 0;
    if (argc == 12 && std::string(argv[11]) != "-") {
      const auto saved = audio_test::read(argv[11]);
      const auto state = unirally::deserialize_title_menu_audio_playback(saved);
      if (unirally::serialize_title_menu_audio_playback(state) != saved)
        throw std::runtime_error("audio file is not canonical");
      playback->restore(state);
      if (unirally::serialize_title_menu_audio_playback(playback->snapshot()) !=
          saved)
        throw std::runtime_error("restored audio state differs");
      if (audio.phase() != unirally::TitleMenuAudioPhase::cued)
        throw std::invalid_argument("saved audio is not in a cued scene");
      first = audio.cued_frame() + 1;
    } else {
      audio.initialize_title();
      while (audio.phase() == unirally::TitleMenuAudioPhase::title_hold) {
        audio.title_frame();
        drain();
      }
      audio.reveal_menu();
      auto action = unirally::AudioCpuMenuAction::waiting;
      while (action == unirally::AudioCpuMenuAction::waiting) {
        action = audio.menu_frame();
        drain();
      }
      const auto exit_clock = audio.cpu_ticks();
      first = static_cast<std::uint32_t>((exit_clock - 306900) / 425568 + 1);
      std::cout << "menu_exit_action=" << unsigned(action)
                << " cpu_clock=" << exit_clock << " frame=" << first << '\n';
    }
    for (auto frame = first; frame <= last; ++frame) {
      const auto found = cues.find(frame);
      audio.cue_frame(frame, found == cues.end() ? unirally::AudioCueList{}
                                                 : found->second);
      drain();
      if (frame == save_frame) {
        const auto state =
            unirally::serialize_title_menu_audio_playback(playback->snapshot());
        audio_test::write(argv[10], state);
        std::cout << "saved_frame=" << frame
                  << " saved_cpu_clock=" << audio.cpu_ticks()
                  << " state_bytes=" << state.size() << '\n';
        return 0;
      }
    }
    audio.finish_pcm_to(std::stoull(argv[7]));
    if (playback) {
      playback->snapshot();
      audio_test::write_pcm(pcm, playback->take_pairs(100000000));
      audio_test::write(argv[10], unirally::serialize_title_menu_audio_playback(
                                      playback->snapshot()));
    } else
      audio_test::write_pcm(pcm, audio.take_pcm());
    if (!events.file)
      throw std::runtime_error("native event output failed");
    std::cout << "computed_cpu_clock=" << audio.cpu_ticks() << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
