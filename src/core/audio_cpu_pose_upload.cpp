#include "audio_cpu_scene.hpp"

namespace unirally {
// $80:F814-F88C and $83:AB25-AB98; R-0075. Five192-byte rows, with the
// fifth falling through rather than making the preceding rows' local call.
void native_audio_upload_pose_work(AudioCpuWorkClock& c, bool lower_buffer) {
    if (!lower_buffer) c.call_local();
    for (unsigned row = 0; row < 5; ++row) {
        c.change_widths();
        c.load_constant(2);
        c.store_port(2);
        c.read_direct(2);
        c.update_register();
        c.load_constant(2);
        c.update_register();
        if (row < 4) c.call_local();
        c.change_widths();
        c.store_port(2);
        c.store_port();
        c.load_constant(2);
        c.store_port(2);
        for (unsigned i = 0; i < 3; ++i) {
            c.load_constant();
            c.store_port();
        }
        c.request_dma(192);
        c.return_local();
    }
    if (!lower_buffer) c.return_far();
}
} // namespace unirally
