#pragma once
#include "audio_cpu_menu_input.hpp"
#include "audio_cpu_text.hpp"
#include "audio_cpu_upload.hpp"
#include "audio_cue.hpp"
#include "audio_engine.hpp"

namespace unirally {
class ClassicContentPack;
struct TitleMenuAudioContent {
    std::array<std::uint8_t, 32> identity{};
    AudioSoundSet score;
    // The race set with each of its songs, by resource 62-66 (R-0077): 62 from pack v32, the
    // others from v33; empty before the pack carries them.
    std::array<AudioSoundSet, race_song_resources> race_songs;
    bool has_race_set() const { return !race_songs[0].tables.empty(); }
    bool has_race_song(unsigned resource) const {
        return resource >= first_race_song_resource
            && resource < first_race_song_resource + race_song_resources
            && !race_songs[resource - first_race_song_resource].score.empty();
    }
    // The medal award's and the gold endings' sets (pack v34; empty before).
    AudioSoundSet award, ending;
    bool has_screen_sets() const { return !award.tables.empty(); }
    AudioPitchData pitch;
    AudioCpuUploadData upload;
    std::array<AudioCpuGraphicsAsset, 128> graphics{};
    std::array<std::uint8_t, 1158> cartridge_defaults{};
    std::array<std::uint8_t, 50> track_types{};
    std::array<std::uint8_t, 12> cartridge_signature{};
    std::vector<std::uint8_t> menu_text;
    std::array<std::uint8_t, 256> characters{};
    std::array<std::uint8_t, 5> arrow_positions{};
    std::array<std::uint8_t, 72> reveal_offsets{};
    std::array<std::uint8_t, 38> reveal_brightness{};
    std::array<std::uint8_t, 17> credits_text{};
    std::array<std::uint8_t, 96> credits_poses{};
    std::vector<std::uint8_t> pose_pointers, pose_frames;
};
enum class TitleMenuAudioPhase : std::uint8_t {
    cold,
    title_hold,
    title_complete,
    menu,
    menu_exit,
    hunter_entry,
    hunter_ready,
    hunter_faded,
    hunter_first_page,
    hunter_page_pressed,
    hunter_timed_wait,
    hunter_credits_ready,
    hunter_credits_prepared,
    hunter_first_pose,
    hunter_second_pose,
    hunter_credits_loop,
    hunter_leaving,
    hunter_reset_ready,
    warm_title_hold,
    warm_title_complete,
    hunter_first_reveal,
    hunter_second_reveal,
    cued // after a menu exit the game reports each frame's queue work (D-0010)
};
struct TitleMenuAudioState {
    TitleMenuAudioPhase phase = TitleMenuAudioPhase::cold;
    AudioCpuMenuAction pending_action = AudioCpuMenuAction::waiting;
    std::array<std::uint8_t, 32> content_identity{};
    AudioEngineState engine;
    AudioCpuQueueState queue;
    AudioCpuSceneWorkState scene;
    AudioCpuTitleHoldState title;
    AudioCpuTextWorkState text;
    AudioCpuMenuInputState menu;
    std::array<std::uint8_t, 8192> cartridge{};
    std::uint16_t hunter_remaining = 30;
    AudioCpuHunterWorkState hunter;
    std::uint32_t cued_frame = 0;            // the last frame cue_frame completed
    std::array<bool, 2> rotation_sounding{}; // the race's `$1003`/`$1005` sound latches
};
// Semantic call boundaries own all continuation state. Controller words are
// external future input; identified content stays immutable outside the state.
class NativeTitleMenuAudio {
public:
    NativeTitleMenuAudio(const TitleMenuAudioContent& content, AudioControllerSource& controllers,
                         AudioEngineEventSink* events = nullptr);
    void set_pcm_sink(AudioPcmSink* sink) { engine_.set_pcm_sink(sink); }
    // Before initialize_title: the cartridge RAM power-on finds (a save file, SAVE-FILES), so the
    // boot's signature check (`$80:8C4E`) keeps it and skips the defaults' work.
    void insert_cartridge(std::span<const std::uint8_t, 8192> image);
    void initialize_title();
    bool title_frame();
    void reveal_menu();
    AudioCpuMenuAction menu_frame();
    bool hunter_entry_frame();
    void hunter_first_fade();
    void hunter_reveal_first_page();
    void hunter_begin_first_page();
    bool hunter_reveal_frame();
    bool hunter_page_wait_frame();
    void hunter_reveal_second_page();
    void hunter_begin_second_page();
    bool hunter_timed_wait_frame();
    std::array<std::uint64_t, 4> hunter_prepare_credits();
    void hunter_build_first_pose();
    void hunter_build_second_pose();
    void hunter_finish_credits_setup();
    bool hunter_credits_frame();
    void hunter_finish_credits();
    void restart_title();
    // From a 1P/2P/VS/OPTIONS menu exit, run frame `frame`'s queue work at its
    // frame-anchored clocks and end at that frame's vertical-blank boundary.
    void cue_frame(std::uint32_t frame, std::span<const AudioCue> cues);
    std::uint64_t cpu_ticks() { return engine_.cpu().ticks(); }
    TitleMenuAudioPhase phase() const { return phase_; }
    std::uint32_t cued_frame() const { return cued_frame_; }
    std::uint8_t menu_selection() const { return menu_.selection; }
    TitleMenuAudioState snapshot();
    void restore(const TitleMenuAudioState& state);
    std::vector<std::int16_t> take_pcm() { return engine_.take_pcm(); }
    void finish_pcm_to(std::uint64_t clocks) { engine_.finish_pcm_to(clocks); }

private:
    void finish_title_initialization();
    const TitleMenuAudioContent* content_;
    NativeAudioEngine engine_;
    TitleMenuAudioPhase phase_ = TitleMenuAudioPhase::cold;
    AudioCpuMenuAction pending_action_ = AudioCpuMenuAction::waiting;
    std::uint16_t hunter_remaining_ = 30;
    AudioCpuHunterWorkState hunter_;
    std::uint32_t cued_frame_ = 0;
    void run_cue(std::uint32_t frame, const AudioCue& cue);
    static bool valid_cued_state(const TitleMenuAudioState& state);
    void load_session(std::uint32_t frame, const AudioCue& cue);
    void rotation_sound(unsigned rider, bool rotating);
    std::array<bool, 2> rotation_sounding_{};
    AudioCpuQueueState queue_;
    AudioCpuSceneWorkState scene_;
    AudioCpuTitleHoldState title_;
    AudioCpuTextWorkState text_;
    AudioCpuMenuInputState menu_;
    std::array<std::uint8_t, 8192> cartridge_;
};
TitleMenuAudioContent title_menu_audio_content(const ClassicContentPack& pack);
std::vector<std::uint8_t> serialize_title_menu_audio(const TitleMenuAudioState& state);
TitleMenuAudioState deserialize_title_menu_audio(std::span<const std::uint8_t> bytes);
} // namespace unirally
