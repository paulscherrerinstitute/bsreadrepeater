/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#include <bsr_json.h>
#include <rapidjson/document.h>
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
};

struct channel_map {
    channel_map() { fprintf(stderr, "struct channel_map CTOR\n"); }
    ~channel_map() { fprintf(stderr, "struct channel_map DESTRUCTOR\n"); }
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
    using rapidjson::Value;
    rapidjson::Document doc;
    try {
        doc.Parse(str, n);
        if (doc.HasParseError()) {
            return 1;
        }
        if (!doc.HasMember("htype")) {
            return 2;
        }
        if (!doc["htype"].IsString()) {
            return 3;
        }
        if (strcmp("bsr_m-1.1", doc["htype"].GetString()) != 0) {
            return 4;
        }
        if (!doc["pulse_id"].IsUint64()) {
            return 5;
        }
        if (doc["global_timestamp"].IsObject()) {
            auto const &o = doc["global_timestamp"];
            if (printc < 20) {
                printc += 1;
                fprintf(stderr, "pulse %" PRIu64 "  ts  sec %" PRIu64 "  ns %" PRIu64 "\n", doc["pulse_id"].GetUint64(),
                        o["sec"].GetUint64(), o["ns"].GetUint64());
                if (doc["NoKey"].IsNull()) {
                    fprintf(stderr, "no key is null\n");
                }
            }
        }
        if (!doc["dh_compression"].IsNull() && !doc["dh_compression"].IsString()) {
            return 6;
        }
        if (doc["dh_compression"].IsString()) {
            char const *c = doc["dh_compression"].GetString();
            if (strcmp("none", c) == 0) {
                header->compr = 0;
            } else if (strcmp("bitshuffle_lz4", c) == 0) {
                header->compr = 1;
            } else if (strcmp("lz4", c) == 0) {
                header->compr = 2;
            } else {
                fprintf(stderr, "WARN compression algo: %s\n", c);
                return 7;
            }
        } else {
            header->compr = 0;
        }
        return 0;
    } catch (std::exception const &e) {
        fprintf(stderr, "ERROR json parse exception\n");
        return -1;
    }
}

extern "C" ERRT json_parse_data_header(char const *str, int n, struct bsread_data_header *header, uint64_t now,
                                       struct channel_map *chnmap, GString **log) {
    rapidjson::Document doc;
    try {
        doc.Parse(str, n);
        if (doc.HasParseError()) {
            return 1;
        }
        if (!doc.HasMember("htype")) {
            return 2;
        }
        if (!doc["htype"].IsString()) {
            return 3;
        }
        if (strcmp("bsr_d-1.1", doc["htype"].GetString()) != 0) {
            return 4;
        }
        if (!doc["channels"].IsArray()) {
            return 5;
        }
        if (chnmap != NULL) {
            auto const &a = doc["channels"].GetArray();
            for (auto it = a.Begin(); it != a.End(); ++it) {
                if (it->IsObject()) {
                    auto const &obj = it->GetObject();
                    if (obj.HasMember("name")) {
                        auto const &n = obj["name"];
                        if (n.IsString()) {
                            if (chnmap->map.contains(n.GetString())) {
                                auto &e = chnmap->map[n.GetString()];
                                float dt = (float)(now - e.ts_last_event);
                                e.emav.update(dt);
                                e.ts_last_event = now;
                                e.dhcount += 1;
                            } else {
                                struct channel_info cin(now);
                                cin.dhcount = 1;
                                chnmap->map[std::string(n.GetString())] = cin;
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
        g_string_append_printf(*out, "  dt ema: %.0f / %.0f", v.second.emav.ema(), v.second.emav.emv());
        g_string_append(*out, "\n");
    }
    return 0;
}
