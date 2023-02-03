/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#include <bsr_json.h>
#include <cmath>
#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <stdio.h>
#include <string>
#include <unordered_map>

// Moving average.
struct emaemv {
    emaemv() {}
    void update(float x);
    float ema() const;
    float emv() const;
    float ema_ = 0;
    float emv_ = 0;
    float k = 0.05;
};

void emaemv::update(float x) {
    float d = x - this->ema_;
    this->ema_ += this->k * d;
    this->emv_ = (1.f - this->k) * (this->emv_ + this->k * d * d);
}

float emaemv::ema() const { return this->ema_; }

float emaemv::emv() const { return this->emv_; }

struct channel_info {
    channel_info() : ts_last_event(0), dhcount(0) {}
    channel_info(uint64_t ts_last_event) : ts_last_event(ts_last_event), dhcount(0){};
    uint64_t ts_last_event;
    uint64_t dhcount;
    struct emaemv emav;
    uint64_t bsread_last_pulse;
    uint64_t bsread_last_timestamp;
};

struct channel_map {
    channel_map() { fprintf(stderr, "TRACE  struct channel_map ctor\n"); }
    ~channel_map() { fprintf(stderr, "TRACE  struct channel_map dtor\n"); }
    std::unordered_map<std::string, struct channel_info> map;
};

static int printc = 0;

extern "C" ERRT json_parse(char const *str, int n, GString **log) {
    using rapidjson::Value;
    rapidjson::Document doc;
    try {
        doc.Parse(str, n);
        if (doc.HasParseError()) {
            fprintf(stderr, "ERROR json parse error\n");
            return -1;
        }
        if (false) {
            for (auto it = doc.MemberBegin(); it != doc.MemberEnd(); ++it) {
                fprintf(stderr, "name %s %d\n", it->name.GetString(), it->value.GetType());
            }
        }
        {
            auto const &v = doc["channels"];
            if (v.IsArray()) {
                auto const &a = v.GetArray();
                for (auto it = a.Begin(); it != a.End(); ++it) {
                    if (it->IsObject()) {
                        auto const &obj = it->GetObject();
                        for (auto it2 = obj.MemberBegin(); it2 != obj.MemberEnd(); ++it2) {
                            if (it2->name == "name") {
                                if (it2->value.IsString()) {
                                    char buf[128];
                                    snprintf(buf, 128, "CHANNEL: %s\n", it2->value.GetString());
                                    *log = g_string_append(*log, buf);
                                }
                            } else if (false) {
                                if (it2->value.IsString()) {
                                    // fprintf(stderr, "STR %s: %s\n", it2->name.GetString(), it2->value.GetString());
                                } else {
                                    // fprintf(stderr, "chn obj key %s\n", it2->name.GetString());
                                }
                            }
                        }
                    } else {
                        // fprintf(stderr, "TY %d\n", it->GetType());
                    }
                }
                // fprintf(stderr, "channel %s\n", v.GetString());
            }
        }
        return 0;
    } catch (std::exception const &e) {
        fprintf(stderr, "ERROR json parse exception\n");
        return -1;
    }
}

extern "C" ERRT json_parse_main_header(char const *str, int n, struct bsread_main_header *header, GString **log) {
    fprintf(stderr, "json_parse_main_header\n");
    using rapidjson::Value;
    rapidjson::Document doc;
    try {
        header->timestamp = 0;
        header->pulse = 0;
        header->compr = 0;
        doc.Parse(str, n);
        if (doc.HasParseError()) {
            return 1;
        }
        if (!doc.IsObject()) {
            return 2;
        }
        Value::ConstMemberIterator mit;
        mit = doc.FindMember("htype");
        if (mit == doc.MemberEnd()) {
            return 2;
        }
        if (!mit->value.IsString()) {
            return 3;
        }
        if (strcmp("bsr_m-1.1", mit->value.GetString()) != 0) {
            return 4;
        }
        mit = doc.FindMember("pulse_id");
        if (mit == doc.MemberEnd()) {
            return 2;
        }
        if (!mit->value.IsUint64()) {
            return 5;
        }
        header->pulse = mit->value.GetUint64();
        mit = doc.FindMember("global_timestamp");
        if (mit == doc.MemberEnd()) {
            return 2;
        }
        if (!mit->value.IsObject()) {
            return 5;
        }
        auto const &gt = mit->value.GetObject();
        mit = gt.FindMember("sec");
        if (mit == doc.MemberEnd()) {
            return 6;
        }
        if (!mit->value.IsUint64()) {
            return 6;
        }
        uint64_t ts_sec = mit->value.GetUint64();
        mit = gt.FindMember("ns");
        if (mit == doc.MemberEnd()) {
            return 6;
        }
        if (!mit->value.IsUint64()) {
            return 6;
        }
        uint64_t ts_ns = mit->value.GetUint64();
        header->timestamp = ts_sec * 1000000000 + ts_ns;
        mit = doc.FindMember("dh_compression");
        if (mit != doc.MemberEnd()) {
            if (mit->value.IsString()) {
                auto const &cs = mit->value.GetString();
                if (strcmp("none", cs) == 0) {
                    header->compr = 0;
                } else if (strcmp("bitshuffle_lz4", cs) == 0) {
                    header->compr = 1;
                } else if (strcmp("lz4", cs) == 0) {
                    header->compr = 2;
                } else {
                    fprintf(stderr, "WARN compression algo: %s\n", cs);
                    return 7;
                }
            }
        }
        if (printc < 20) {
            printc += 1;
            fprintf(stderr, "%" PRIu64 "\n", header->timestamp);
        }
        return 0;
    } catch (std::exception const &e) {
        fprintf(stderr, "ERROR json parse exception\n");
        return -1;
    }
}

extern "C" ERRT json_parse_data_header(char const *str, int n, struct bsread_data_header *header, uint64_t now,
                                       struct channel_map *chnmap, struct bsread_main_header *mh, GString **log) {
    using rapidjson::Value;
    rapidjson::Document doc;
    try {
        header->channel_count = 0;
        doc.Parse(str, n);
        if (doc.HasParseError()) {
            return 1;
        }
        if (!doc.IsObject()) {
            return 1;
        }
        Value::ConstMemberIterator mit;
        mit = doc.FindMember("htype");
        if (mit == doc.MemberEnd()) {
            return 2;
        }
        if (!mit->value.IsString()) {
            return 2;
        }
        if (strcmp("bsr_d-1.1", mit->value.GetString()) != 0) {
            return 2;
        }
        mit = doc.FindMember("channels");
        if (mit == doc.MemberEnd()) {
            return 3;
        }
        if (!mit->value.IsArray()) {
            return 3;
        }
        auto const &chans_val = mit->value;
        if (chnmap != NULL) {
            auto const &a = chans_val.GetArray();
            for (auto it = a.Begin(); it != a.End(); ++it) {
                if (it->IsObject()) {
                    auto const &obj = it->GetObject();
                    auto const &name_mem = obj.FindMember("name");
                    if (name_mem != obj.MemberEnd()) {
                        if (name_mem->value.IsString()) {
                            auto const &name = name_mem->value.GetString();
                            header->channel_count += 1;
                            if (chnmap->map.contains(name)) {
                                auto &e = chnmap->map[name];
                                float dt = (float)(now - e.ts_last_event);
                                e.emav.update(dt);
                                e.ts_last_event = now;
                                e.dhcount += 1;
                                e.bsread_last_timestamp = mh->timestamp;
                                e.bsread_last_pulse = mh->pulse;
                            } else {
                                struct channel_info cin(now);
                                cin.ts_last_event = now;
                                cin.dhcount = 1;
                                cin.bsread_last_timestamp = mh->timestamp;
                                cin.bsread_last_pulse = mh->pulse;
                                chnmap->map[std::string(name)] = cin;
                            }
                        }
                    }
                }
            }
        }
        return 0;
    } catch (std::exception const &e) {
        fprintf(stderr, "ERROR json parse exception\n");
        return -1;
    }
}

extern "C" uint64_t now_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

extern "C" struct channel_map *channel_map_new() {
    auto ret = new channel_map();
    return ret;
}

extern "C" void channel_map_release(struct channel_map *self) { delete self; }

extern "C" ERRT channel_map_str(struct channel_map *self, GString **out) {
    for (auto const &v : self->map) {
        if ((*out)->len + 512 > (*out)->allocated_len) {
            gsize l1 = (*out)->len;
            *out = g_string_set_size(*out, (*out)->allocated_len + 512);
            *out = g_string_set_size(*out, l1);
        }
        g_string_append_printf(*out, "name: %s", v.first.c_str());
        g_string_append_printf(*out, "  dhcount: %" PRIu64, v.second.dhcount);
        g_string_append_printf(*out, "  dt ema: %.0f / %.0f", v.second.emav.ema(), sqrtf(v.second.emav.emv()));
        g_string_append_printf(*out, "  pulse: %" PRIu64, v.second.bsread_last_pulse);
        g_string_append_printf(*out, "  ts: %" PRIu64, v.second.bsread_last_timestamp);
        g_string_append(*out, "\n");
    }
    return 0;
}

extern "C" ERRT stats_source_to_json(struct stats_source_pub *stats, GString **out) {
#define MEMU64(name) doc.AddMember(#name, stats->name, doc.GetAllocator());
#define MEMF32(name)                                                                                                   \
    if (std::isinf(stats->name) || std::isnan(stats->name)) {                                                          \
        doc.AddMember(#name, rapidjson::Value(), doc.GetAllocator());                                                  \
    } else {                                                                                                           \
        doc.AddMember(#name, stats->name, doc.GetAllocator());                                                         \
    }
    rapidjson::Document doc;
    doc.SetObject();
    MEMU64(received);
    MEMU64(mhparsed);
    MEMU64(dhparsed);
    MEMU64(json_parse_errors);
    MEMU64(dhdecompr);
    MEMU64(sentok);
    MEMU64(recv_bytes);
    MEMU64(sent_bytes);
    MEMU64(bsread_errors);
    MEMU64(input_reopened);
    MEMU64(throttling_enable_count);
    MEMU64(timestamp_out_of_range_count);
    MEMU64(unexpected_frames_count);
    MEMF32(mpmsglen_ema);
    MEMF32(mpmsglen_emv);
    MEMF32(msg_dt_ema);
    MEMF32(msg_dt_emv);
    MEMF32(msg_emit_age_ema);
    MEMF32(msg_emit_age_emv);
    MEMF32(msg_emit_age_min);
    MEMF32(msg_emit_age_max);
    rapidjson::StringBuffer buf;
    rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<char>, rapidjson::UTF8<char>, rapidjson::CrtAllocator, 2>
        writer(buf, 0);
    if (!doc.Accept(writer)) {
        return -1;
    }
    writer.Flush();
    buf.GetString();
    // int n = write(2, buf.GetString(), buf.GetSize());
    // fprintf(stderr, "\nstr debug out %d %ld %ld\n", n, buf.GetLength(), buf.GetSize());
    g_string_append(*out, buf.GetString());
    return 0;
#undef MEMU64
#undef MEMF32
}
