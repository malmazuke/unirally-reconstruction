#include "zoom_zoo_movement.hpp"
#include "vertical_contact.hpp"
#include <array>
#include <stdexcept>

static void require(bool value) {if(!value)throw std::runtime_error("ZOOM ZOO trial expectation failed");}
template<class F> static void rejects(F action) {
    bool rejected=false;try{action();}catch(const std::invalid_argument&){rejected=true;}require(rejected);
}
int main() {
    using namespace unirally;
    // Review case delayed Right exposed the omitted low-speed clear. The
    // original interval is asymmetric: -16 clears, +16 increments.
    for(int speed:{-16,-1,0,15})
        require(next_wrong_direction_counter(7,static_cast<std::uint16_t>(speed),0x4000,2)==0);
    for(int speed:{-17,16})
        require(next_wrong_direction_counter(7,static_cast<std::uint16_t>(speed),0x4000,2)==8);
    require(next_wrong_direction_counter(7,100,0x4000,1)==0);
    require(next_wrong_direction_counter(7,100,0xc000,2)==0);
    require(next_wrong_direction_counter(7,100,0,0)==8);
    rejects([]{(void)next_wrong_direction_counter(179,100,0x4000,2);});
    require(next_wrong_direction_counter(179,100,0x4000,2,true)==120);
    require(next_wrong_direction_counter(179,100,0,0,true)==120);
    require(next_wrong_direction_counter(178,100,0,0,true)==179);
    require(next_wrong_direction_counter(179,0,0,0,true)==0);
    ZoomZooState state{};state.movement.frame=1649;state.opponent_retained_oam_x=101;
    state.reflection[1].step=4;state.reflection[1].end=9;state.reflection[1].air_turns=3;
    auto encoded=serialize_zoom_zoo(state);require(encoded.size()==395);
    require(serialize_zoom_zoo(deserialize_zoom_zoo(encoded))==encoded);
    auto invalid=encoded;invalid.pop_back();rejects([&]{(void)deserialize_zoom_zoo(invalid);});
    invalid=encoded;invalid[393]=3;rejects([&]{(void)deserialize_zoom_zoo(invalid);});
    invalid=encoded;invalid[363]=17;rejects([&]{(void)deserialize_zoom_zoo(invalid);});
    ControllerButtons input{};input.left=true;
    rejects([&]{update_zoom_zoo(state,input,{});});require(serialize_zoom_zoo(state)==encoded);
    input={};state.movement.frame=1849;
    rejects([&]{update_zoom_zoo(state,input,{});});

    // M4-15: malformed newly serialized future state must fail before update.
    ZoomZooState race{};race.complete_race=race.sustained=true;race.movement.frame=1649;
    for(auto& lap:race.race.riders)lap.laps_remaining=4;
    auto race_bytes=serialize_zoom_zoo(race);require(race_bytes.size()==565);
    require(serialize_zoom_zoo(deserialize_zoom_zoo(race_bytes))==race_bytes);
    for(unsigned offset:{429U,433U,451U,455U,511U,513U,553U,555U,561U,563U}) {
        auto corrupt=race_bytes;corrupt[offset]=2;
        rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
    }
    for(unsigned offset:{529U,548U}) {
        auto corrupt=race_bytes;corrupt[offset]=1;
        rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
    }
    for(unsigned offset:{549U,557U}) {
        auto corrupt=race_bytes;corrupt[offset]=49;corrupt[offset+2]=1;
        rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
    }
    for(unsigned offset:{551U,553U,555U,559U,561U,563U}) {
        auto corrupt=race_bytes;corrupt[offset]=1;
        rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
    }
    for(unsigned offset:{423U,445U}) {
        auto corrupt=race_bytes;corrupt[offset]=0;
        rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
    }
    auto premature_delay=race_bytes;premature_delay[515]=1;
    rejects([&]{(void)deserialize_zoom_zoo(premature_delay);});
    for(unsigned offset:{521U,523U}) {
        auto corrupt=race_bytes;corrupt[offset]=17;
        rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
    }

    // M4-16 review: phase and result mutations must be rejected, rather
    // than accepted with changed future countdown or winner/graph semantics.
    std::array<std::uint8_t,11> header{};header[3]=header[7]=64;header[5]=header[9]=33;
    ZoomZooContent initial_content{};initial_content.movement.sampling.track=header;
    std::array<std::uint8_t,26> weights{};weights[0]=4;initial_content.reward_weights=weights;
    auto initial=classic_crawler_zoom_zoo_start(initial_content);
    const auto initial_bytes=serialize_zoom_zoo(initial);
    auto paused=initial;paused.movement.frame=1500;paused.pause.selection=0xffffU;paused.pause.released=1;
    restart_zoom_zoo(paused,initial_content);
    require(serialize_zoom_zoo(paused)==initial_bytes);
    paused=initial;paused.movement.frame=1500;paused.pause.selection=0xffffU;paused.pause.released=1;paused.fade_level=30;
    ControllerButtons restart_buttons{};restart_buttons.start=true;
    update_zoom_zoo(paused,restart_buttons,initial_content);
    require(serialize_zoom_zoo(paused)==initial_bytes);
    require(initial_bytes.size()==742);
    require(serialize_zoom_zoo(deserialize_zoom_zoo(initial_bytes))==initial_bytes);
    // $82DB87-DB94 copies one 26-byte $82D7A4 template into both learned
    // banks, so the opponent's event-one weight follows the static content
    // rather than a constant repeated in the initializer.
    require(initial.movement.rewards.event_one_weight==4);
    {
        std::array<std::uint8_t,26> other{};other[0]=6;other[1]=9;
        auto other_content=initial_content;other_content.reward_weights=other;
        const auto started=classic_crawler_zoom_zoo_start(other_content);
        require(started.movement.rewards.event_one_weight==6);
        require(started.player_announcements.queue.event_one_weight==6);
        require(started.learned_weights[0][0]==9 && started.learned_weights[1][0]==9);
    }
    for(unsigned offset:{565U,567U,569U,581U,583U}) {
        auto corrupt=initial_bytes;corrupt[offset]^=1;
        rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
    }
    for(unsigned offset:{585U,617U,618U,619U,621U,623U,624U,626U,628U,630U,632U,634U,636U,638U,640U,642U,644U,646U,648U,650U,652U,654U,656U}) {
        auto corrupt=initial_bytes;corrupt[offset]^=1;
        rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
    }
    for(unsigned offset:{730U,732U,734U,738U}) {
        auto corrupt=initial_bytes;corrupt[offset]=2;
        rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
    }
    auto result=initial;result.movement.frame=7000;result.movement.countdown=0;
    result.fade_level=30;result.start_boost.fill(0);result.result_updates=115;
    result.race.finish_delay=240;
    for(unsigned i=0;i<2;++i) {
        result.race.riders[i].laps_remaining=0;result.race.riders[i].finished=1;
        for(unsigned lap=0;lap<3;++lap)result.race.lap_times[i][lap]=3000;
        result.race.total_times[i]=9000;
    }
    result.result.graph_minimum=2800;result.result.graph_maximum=3000;result.result.published_totals.fill(9000);
    auto result_bytes=serialize_zoom_zoo(result);
    require(serialize_zoom_zoo(deserialize_zoom_zoo(result_bytes))==result_bytes);
    {
        // $81:C73E-C75B: at the 10:00 clock limit both riders are finished
        // whatever their laps; an unfinished lap count keeps the no-time total.
        auto timeout=result;timeout.movement.timer={9,5,9,9,2};
        timeout.race.riders[0].laps_remaining=2;
        for(unsigned lap=1;lap<3;++lap)timeout.race.lap_times[0][lap]=60000;
        timeout.race.total_times[0]=60000;timeout.result.published_totals={60000,9000};
        const auto timeout_bytes=serialize_zoom_zoo(timeout);
        require(serialize_zoom_zoo(deserialize_zoom_zoo(timeout_bytes))==timeout_bytes);
        auto early=timeout;early.movement.timer={9,5,9,8,4};
        rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(early));});
        // The limit finishes both riders on the same update, so a lap-short
        // finish beside an unfinished rider is not reachable.
        auto one_finished=timeout;one_finished.race.riders[1].finished=0;one_finished.race.riders[1].laps_remaining=1;
        one_finished.race.lap_times[1][2]=60000;one_finished.race.total_times[1]=60000;
        one_finished.result.published_totals={60000,60000};
        rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(one_finished));});
        auto lap_complete=one_finished;lap_complete.race.riders[0].laps_remaining=0;
        lap_complete.race.lap_times[0][1]=lap_complete.race.lap_times[0][2]=3000;lap_complete.race.total_times[0]=9000;
        lap_complete.result.published_totals={9000,60000};
        require(serialize_zoom_zoo(deserialize_zoom_zoo(serialize_zoom_zoo(lap_complete)))==serialize_zoom_zoo(lap_complete));
        auto timed_total=timeout;timed_total.race.total_times[0]=3000;
        rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(timed_total));});
    }
    for(unsigned value:{88U,199U,200U,215U,216U,255U}) {
        auto corrupt=result_bytes;corrupt[585]=static_cast<std::uint8_t>(value);
        rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
    }
    for(unsigned value:{72U,87U,88U,199U,216U,255U}) {
        auto corrupt=result_bytes;corrupt[288]=static_cast<std::uint8_t>(value);
        rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
    }
    for(unsigned value:{72U,87U}) {
        auto valid=result_bytes;valid[585]=static_cast<std::uint8_t>(value);
        require(serialize_zoom_zoo(deserialize_zoom_zoo(valid))==valid);
    }
    for(unsigned value:{200U,215U}) {
        auto valid=result_bytes;valid[288]=static_cast<std::uint8_t>(value);
        require(serialize_zoom_zoo(deserialize_zoom_zoo(valid))==valid);
    }
    for(unsigned offset:{467U,487U,507U,509U,573U,575U,577U,579U}) {
        auto corrupt=result_bytes;corrupt[offset]=0x60;corrupt[offset+1]=0xea;
        rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
    }

    // Frozen late-roll recovery now admits charge160 and binary bounce-active.
    // Invalid charge values and still-unproduced prior steps remain rejected.
    for(unsigned offset:{644U,654U,668U,678U}) {
        auto corrupt=result_bytes;corrupt[offset]=1;
        rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
    }
    {
        // Rider 0 may be mid-bounce; a nonbinary flag still rejects.
        auto corrupt=result_bytes;corrupt[650]=2;
        rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
        auto valid=result_bytes;valid[650]=1;
        require(serialize_zoom_zoo(deserialize_zoom_zoo(valid))==valid);
    }
    // $829641-9649 gates the charge store on rider $0FF9 whenever $0C6D is
    // non-zero, and the reference guard holds $0C6D at 1 throughout, so the
    // opponent can neither charge nor bounce. Both of its flags reject.
    for(unsigned value:{1U,2U}) {
        auto corrupt=result_bytes;corrupt[674]=static_cast<std::uint8_t>(value);
        rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
    }
    auto charged=result;charged.rolls[0].bounce_charge=160;
    require(serialize_zoom_zoo(deserialize_zoom_zoo(serialize_zoom_zoo(charged)))==serialize_zoom_zoo(charged));
    charged.rolls[0].step=1;
    rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(charged));});
    auto opponent_charged=result;opponent_charged.rolls[1].bounce_charge=160;
    rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(opponent_charged));});
    auto opponent_bouncing=result;opponent_bouncing.rolls[1].bounce_active=1;
    rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(opponent_bouncing));});
    // The opponent's reward queue now carries the player's bounds: a weight
    // halving can never reach, or a cooldown above the 40 of $81C2CC-C2D0.
    for(unsigned weight:{0U,3U,250U}) {
        auto forged=result;forged.movement.rewards.event_one_weight=static_cast<std::uint8_t>(weight);
        rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(forged));});
    }
    {
        auto forged=result;forged.movement.rewards.cooldown=60000;
        rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(forged));});
    }
    // $83E1F1 masks the sloped selector with 7 and the flat path writes only
    // 0 or 1. Removing the multi-axis guards left this field unbounded, so a
    // forged 14 or 65535 would alias onto 6 and 7 and play identically.
    for(unsigned value:{8U,14U,255U,65535U}) {
        auto forged=result;forged.movement.opponent_ai.trick_selector=static_cast<std::uint16_t>(value);
        rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(forged));});
    }
    for(unsigned value:{0U,1U,2U,6U,7U}) {
        auto valid=result;valid.movement.opponent_ai.trick_selector=static_cast<std::uint16_t>(value);
        const auto bytes=serialize_zoom_zoo(valid);
        require(serialize_zoom_zoo(deserialize_zoom_zoo(bytes))==bytes);
    }
    // $81C598-C5C8 never publishes a zero, so a restore carrying one would
    // otherwise be accepted and only fail on the following update.
    {
        auto forged=result;auto& o=forged.movement.rewards;
        o.read_cursor=0;o.write_cursor=2;o.entries[1]=0;o.entries[2]=1;
        rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(forged));});
        o.entries[1]=1;
        require(serialize_zoom_zoo(deserialize_zoom_zoo(serialize_zoom_zoo(forged)))==serialize_zoom_zoo(forged));
    }
    auto bad_weight=initial;bad_weight.learned_weights[0][0]=1;
    const auto weight_before=serialize_zoom_zoo(bad_weight);
    rejects([&]{validate_zoom_zoo_content_state(bad_weight,initial_content);});
    rejects([&]{update_zoom_zoo(bad_weight,{},initial_content);});
    require(serialize_zoom_zoo(bad_weight)==weight_before);
    auto active_roll=result;active_roll.rolls[0].step=0xffff;
    active_roll.rolls[0].pose_base=0x8000;
    rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(active_roll));});
    auto held_roll=result;held_roll.rolls[0].step=0xfffb;
    held_roll.rolls[0].held_updates=7;held_roll.rolls[0].held_rotations=6;
    rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(held_roll));});
    // Previous held rotations survive a return/new roll; equality is not required.
    held_roll.rolls[0].held_updates=6;held_roll.rolls[0].held_rotations=7;
    require(serialize_zoom_zoo(deserialize_zoom_zoo(serialize_zoom_zoo(held_roll)))==serialize_zoom_zoo(held_roll));
    held_roll.rolls[0].held_rotations=65000;
    rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(held_roll));});
    auto impossible=result;impossible.movement.frame=1376;impossible.movement.countdown=270;
    impossible.fade_level=0;impossible.start_boost.fill(384);impossible.result_updates=1;impossible.result={};
    rejects([&]{(void)deserialize_zoom_zoo(serialize_zoom_zoo(impossible));});
    const auto before_early_restart=serialize_zoom_zoo(initial);
    rejects([&]{restart_zoom_zoo(initial,initial_content);});
    require(serialize_zoom_zoo(initial)==before_early_restart);
    restart_zoom_zoo(result,initial_content);
    require(serialize_zoom_zoo(result)==initial_bytes);

    // A restored descriptor must not index checkpoint flags before validation.
    race.movement.riders[0].contact.selected_word=0x03f0;
    std::array<std::uint8_t,20> checkpoint_flags{};
    ZoomZooContent checkpoint_content{};
    checkpoint_content.movement.flat_contact.flags=checkpoint_flags;
    const auto malformed_race=serialize_zoom_zoo(race);
    rejects([&]{update_zoom_zoo(race,{},checkpoint_content);});
    require(serialize_zoom_zoo(race)==malformed_race);

    // Auxiliary boundary return increments only the low byte and preserves the
    // precorrection position. Ordinary unsupported contact increments a word.
    RiderContactState rider{};rider.unsupported_count=8;rider.unsupported_duration=0x12ff;
    rider.previous_uncorrected_x=123;rider.previous_uncorrected_y=456;
    ContactMotion motion{};motion.x=900;motion.y=800;motion.velocity_x=0xff80;
    VerticalContactSummary summary{};summary.boundary_marker=true;summary.any_nonnegative_probe=true;summary.penetration=127;
    resolve_vertical_contact(rider,motion,summary,{0,true,0,0xc200},{},{});
    require(rider.unsupported_duration==0x1200 && rider.unsupported_count==9);
    require(rider.previous_uncorrected_x==123 && motion.y==800 && motion.velocity_x==0xff80);
    summary={};resolve_vertical_contact(rider,motion,summary,{0,true,0,0xc200},{},{});
    require(rider.unsupported_duration==0x1201 && rider.auxiliary_flag==0 && rider.previous_uncorrected_x==900);

    // A negative arithmetic shift rounds toward minus infinity before the
    // wrapped unsigned multiply and negative-slope +1 correction.
    std::array<std::uint8_t,9> shifts{},multipliers{};shifts[4]=2;multipliers[4]=3;
    rider={};motion={};motion.velocity_x=0xfff9;summary={};summary.supported=true;
    summary.any_nonnegative_probe=true;summary.angle=-4;summary.penetration=3;
    resolve_vertical_contact(rider,motion,summary,{0,true,0,0xc200},shifts,multipliers);
    require(motion.velocity_y==7 && motion.velocity_x==0xfff7 && motion.y==0xfffd);
    const auto previous_y=motion.y;summary.angle=9;
    rejects([&]{resolve_vertical_contact(rider,motion,summary,{0,true,0,0xc200},shifts,multipliers);});
    require(motion.y==previous_y);
    // A first probe may establish support. A later equally deep probe wins
    // in source order and clears the leading-probe predicate.
    std::array<std::uint8_t,64> columns{};
    for(unsigned i=0;i<columns.size();i+=2)columns[i]=0xa0;
    columns[32]=4;
    std::array<std::uint8_t,2> flags{};
    CollisionPoints points{};TrackSamples samples{};
    points[0].y=5;samples[0]=2;
    auto first=summarize_vertical_contact({columns,flags},points,samples,0,0);
    require(first.supported && first.leading_support && first.penetration==2);
    points[2].y=5;samples[2]=2;
    auto later=summarize_vertical_contact({columns,flags},points,samples,0,0);
    require(later.supported && !later.leading_support && later.penetration==2);

    // M4-13: the matrix uses the initial displacement angle; the orientation
    // impulse uses the subsequent quadrant clamp. Authored coordinates differ
    // from the captured landings, and both impulse signs are exercised.
    std::array<std::uint8_t,1512> matrices{};
    auto landing=[&](int dx,int dy,int angle) {
        RiderContactState contact{};contact.unsupported_count=9;
        contact.unsupported_duration=60;contact.previous_uncorrected_x=500;
        contact.previous_uncorrected_y=500;
        ContactMotion moved{};moved.x=static_cast<std::uint16_t>(500+dx);
        moved.y=static_cast<std::uint16_t>(500+dy);moved.previous_x_displacement=19;
        VerticalContactSummary surface{};surface.supported=true;
        surface.any_nonnegative_probe=true;surface.angle=static_cast<std::int16_t>(angle);
        resolve_vertical_contact(contact,moved,surface,{1,false,0,0xc200},shifts,multipliers,matrices);
        require(contact.recontact && contact.unsupported_count==0);
        return moved.orientation_impulse;
    };
    require(landing(18,18,-3)==5);
    require(landing(-18,-18,0)==static_cast<std::uint16_t>(-5));
    require(landing(-18,18,0)==0);
    require(landing(18,1,-3)==5); // zero half-dy retains horizontal endpoint four
    require(landing(0,18,0)==0); // zero dx clamps vertical endpoint to 31
    require(landing(0,0,0)==5); // equal zero displacements use horizontal path

    // M4-14: full continuation fields must survive an independent restore.
    ZoomZooState extended{};extended.sustained=true;extended.movement.frame=2500;
    extended.surface[0].mode=1;extended.surface[0].angle=0xffe4;
    extended.surface[1].leading_support=1;extended.surface[1].animation_delta=0xfffe;
    const auto extended_bytes=serialize_zoom_zoo(extended);
    require(extended_bytes.size()==423 && deserialize_zoom_zoo(extended_bytes).surface[1].animation_delta==0xfffe);
    auto corrupt=extended_bytes;corrupt[0]='X';rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
    for(unsigned rider_index=0;rider_index<2;++rider_index) {
        for(unsigned field_offset:{0U,4U,6U,8U,12U}) {
            corrupt=extended_bytes;corrupt[395+14*rider_index+field_offset]=2;
            rejects([&]{(void)deserialize_zoom_zoo(corrupt);});
        }
    }

    // Inverted vertical probes complement local Y and correct it upward in
    // source coordinates. Horizontal probes swap axes and retain direction.
    samples={};points={};samples[0]=0x8002;points[0].y=10;
    auto inverted=summarize_vertical_contact({columns,flags},points,samples,0,0);
    require(inverted.penetration==2 && inverted.inverted_vertical && inverted.leading_support);
    flags[1]=1;points[0].x=5;points[0].y=0;samples[0]=2;
    auto side=summarize_vertical_contact({columns,flags},points,samples,0,0);
    require(side.horizontal_penetration==2 && side.horizontal_direction==4 && side.penetration==0);

    // The 31-unit endpoint preserves airborne timing and uses arithmetic /4,
    // while a steep 28-unit contact can derive horizontal motion from vertical.
    std::array<std::uint8_t,32> full_shifts{},full_multipliers{};
    full_shifts[28]=1;full_multipliers[28]=1;
    rider={};rider.unsupported_count=1;motion={};motion.velocity_x=0xfff9;
    summary={};summary.supported=true;summary.any_nonnegative_probe=true;summary.angle=31;
    resolve_vertical_contact(rider,motion,summary,{0,false,0,0},full_shifts,full_multipliers);
    require(motion.velocity_x==0xfffe && rider.unsupported_count==2 && rider.unsupported_duration==1);
    rider={};motion={};motion.velocity_y=40;summary.angle=28;
    resolve_vertical_contact(rider,motion,summary,{0,false,0,0},full_shifts,full_multipliers);
    require(motion.velocity_x==10 && motion.velocity_y==40);

    // R-0033 review landing: the same steep face after nine unsupported
    // updates clears airtime and preserves incoming velocity ($81:9309).
    rider={};rider.unsupported_count=9;rider.unsupported_duration=30;
    motion={};motion.velocity_x=48;motion.velocity_y=40;
    resolve_vertical_contact(rider,motion,summary,{0,false,0,0},full_shifts,full_multipliers);
    require(motion.velocity_x==48 && motion.velocity_y==40 && rider.recontact);
    require(rider.unsupported_count==0 && rider.unsupported_duration==0);

}
