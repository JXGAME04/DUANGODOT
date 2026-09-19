#include "jx/zone/KMapInstance.h"

#include <algorithm>

#include <fmt/format.h>

#include "jx/client.pb.h"
#include "jx/log.hpp"
#include "jx/msg.pb.h"

namespace jx::zone {

KMapInstance::KMapInstance(std::uint32_t instance_id, KSubWorldConfig cfg)
    : instance_id_(instance_id), world_(std::move(cfg))
{
    tick_timing_ = &core::metrics().timing(fmt::format("map.{}.tick", world_.map_id()));
    drain_metric_ = fmt::format("map.{}.tick.drain_network", world_.map_id());
}

void KMapInstance::tick()
{
    const Nanos t0 = steady_now();

    // 1. drain the commands the outside asked for (SPEC 22 phase 1 + 2)
    {
        core::ScopedTiming t(core::metrics().timing(drain_metric_));
        commands_ += inbox_.drain([this](KWorldCommand cmd) { apply(cmd); });
    }

    // 2. simulate (movement, spatial, ai, combat, interest, snapshot - inside KSubWorld)
    world_.tick();

    // 3. a trap script asked to move somebody to another map: take the player out here and
    //    hand the whole state to the server, which gives it to the target instance (SPEC 63)
    for (const KWorldChange& change : world_.take_world_changes()) {
        KEvWorldChange ev;
        ev.sid = change.sid;
        ev.map_id = change.map_id;
        ev.pos = change.pos;
        if (!world_.role_snapshot(change.sid, ev.role)) continue;
        world_.remove_player(change.sid);
        events_.push(std::move(ev));
    }

    const double ms = static_cast<double>((steady_now() - t0).count()) / 1e6;
    note_tick_cost(ms);
    if (tick_timing_ != nullptr) tick_timing_->add_ns(static_cast<std::uint64_t>(ms * 1e6));
}

void KMapInstance::apply(KWorldCommand& cmd)
{
    std::visit(overloaded{
                   [this](KCmdSpawnPlayer& c) {
                       KEvSessionOpened ev;
                       ev.sid = c.sid;
                       ev.conn_id = c.conn_id;
                       ev.reason = c.reason;
                       EntityId entity;
                       Pos pos;
                       ev.result = world_.spawn_player(c.sid, c.role, entity, pos, c.has_at ? &c.at : nullptr);
                       if (ev.result == pb::RESULT_OK) {
                           ev.entity = entity;
                           ev.pos = pos;
                           ev.map_id = world_.map_id();
                           ev.scene_w = static_cast<std::uint32_t>(world_.config().width);
                           ev.scene_h = static_cast<std::uint32_t>(world_.config().height);
                       }
                       events_.push(std::move(ev));
                   },
                   [this](KCmdRemovePlayer& c) {
                       if (c.save) emit_save(c.sid, true);
                       world_.remove_player(c.sid);
                   },
                   [this](KCmdClientPacket& c) { apply_client_packet(c); },
                   [this](KCmdSaveRequest& c) { emit_save(c.sid, c.final); },
               },
               cmd);
}

void KMapInstance::apply_client_packet(const KCmdClientPacket& cmd)
{
    switch (cmd.msg_id) {
    case pb::C2G_MOVE: {
        pb::MoveReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.move_request(cmd.sid, Pos{req.target().x(), req.target().y()}, req.seq());
        break;
    }
    case pb::C2G_ATTACK: {
        pb::AttackReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.attack_request(cmd.sid, EntityId{req.target()}, req.seq());
        break;
    }
    case pb::C2G_CHAT: {
        pb::ChatReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.chat(cmd.sid, req.text());
        break;
    }
    case pb::C2G_ITEM_MOVE: {
        pb::ItemMove req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.item_move_request(cmd.sid, req.id(), static_cast<int>(req.room()), static_cast<int>(req.x()), static_cast<int>(req.y()), req.seq());
        break;
    }
    case pb::C2G_ITEM_EQUIP: {
        pb::ItemEquipReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.item_equip_request(cmd.sid, req.id(), req.part(), req.seq());
        break;
    }
    case pb::C2G_ITEM_UNEQUIP: {
        pb::ItemUnequipReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.item_unequip_request(cmd.sid, static_cast<int>(req.part()), req.seq());
        break;
    }
    case pb::C2G_ITEM_USE: {
        pb::ItemUseReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.item_use_request(cmd.sid, req.id(), req.seq());
        break;
    }
    case pb::C2G_ITEM_DROP: {
        pb::ItemDropReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.item_drop_request(cmd.sid, req.id(), req.seq());
        break;
    }
    case pb::C2G_PICK_UP: {
        pb::PickUpReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.pick_up_request(cmd.sid, EntityId{req.entity_id()}, req.seq());
        break;
    }
    case pb::C2G_ADD_POINT: {
        pb::AddPointReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.add_point_request(cmd.sid, static_cast<int>(req.attribute()), static_cast<int>(std::min<std::uint32_t>(req.points(), 1000u)), req.seq());
        break;
    }
    case pb::C2G_CAST_SKILL: {
        pb::CastSkillReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        const bool at_target = req.target() != 0;
        world_.cast_skill_request(cmd.sid, static_cast<int>(std::min<std::uint32_t>(req.skill_id(), 2000u)), at_target ? -1 : req.x(),
                                  at_target ? 0 : req.y(), EntityId{req.target()}, req.seq());
        break;
    }
    case pb::C2G_REVIVE: {
        pb::ReviveReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.revive_request(cmd.sid, req.seq());
        break;
    }
    case pb::C2G_RIDE: {
        pb::RideReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.ride_request(cmd.sid, req.on(), req.seq());
        break;
    }
    case pb::C2G_SIT: {
        pb::SitReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.sit_request(cmd.sid, req.sit(), req.seq());
        break;
    }
    case pb::C2G_PK_STATE: {
        pb::PKStateReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.pk_state_request(cmd.sid, req.state());
        break;
    }
    case pb::C2G_TRADE: {
        pb::TradeReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.trade_request(cmd.sid, static_cast<int>(req.cmd()), EntityId{req.target()}, req.arg(), req.text());
        break;
    }
    case pb::C2G_TEAM: {
        pb::TeamReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.team_request(cmd.sid, static_cast<int>(req.cmd()), EntityId{req.target()}, req.flag());
        break;
    }
    case pb::C2G_SET_AURA: {
        pb::SetAuraReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.set_aura_request(cmd.sid, static_cast<int>(std::min<std::uint32_t>(req.skill_id(), 2000u)));
        break;
    }
    case pb::C2G_SKILL_DESC: {
        pb::SkillDescReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.skill_desc_request(cmd.sid, static_cast<int>(std::min<std::uint32_t>(req.skill_id(), 2000u)),
                                  static_cast<int>(std::min<std::uint32_t>(req.level(), 64u)));
        break;
    }
    case pb::C2G_ADD_SKILL_POINT: {
        pb::AddSkillPointReq req;
        if (!req.ParseFromString(cmd.payload)) break;
        world_.add_skill_point_request(cmd.sid, static_cast<int>(std::min<std::uint32_t>(req.skill_id(), 2000u)),
                                       static_cast<int>(std::min<std::uint32_t>(req.points(), 1000u)), req.seq());
        break;
    }
    default:
        log::debug("zone", "client message not handled by the world",
                   {log::kv("sid", cmd.sid), log::kv("msg", cmd.msg_id), log::kv("map", world_.map_id())});
        break;
    }
}

void KMapInstance::emit_save(std::uint64_t sid, bool final)
{
    KEvPlayerSave save;
    save.sid = sid;
    save.final = final;
    save.tick = world_.tick_count();
    if (!world_.role_snapshot(sid, save.role)) return;
    events_.push(std::move(save));
}

} // namespace jx::zone
