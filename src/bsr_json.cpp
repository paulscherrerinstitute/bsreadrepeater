/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#include <bsr_json.h>
#include <rapidjson/document.h>
#include <stdio.h>

extern "C" ERRT json_parse(char const *str, int n, GString *log) {
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
                                    g_string_append(log, buf);
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

extern "C" ERRT json_parse_main_header(char const *str, int n, struct bsread_main_header *header, GString *log) {
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
        if (!doc["pulse_id"].IsInt64()) {
            return 5;
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

extern "C" ERRT json_parse_data_header(char const *str, int n, struct bsread_data_header *header, GString *log) {
    if (header->channels != NULL) {
        g_array_set_size(header->channels, 0);
    }
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
        if (strcmp("bsr_d-1.1", doc["htype"].GetString()) != 0) {
            return 4;
        }
        if (!doc["channels"].IsArray()) {
            return 5;
        }
        auto const &a = doc["channels"].GetArray();
        for (auto it = a.Begin(); it != a.End(); ++it) {
            if (it->IsObject()) {
                auto const &obj = it->GetObject();
                if (obj.HasMember("name")) {
                    auto const &n = obj["name"];
                    if (n.IsString()) {
                        if (header->channels != NULL) {
                            char const *s1 = n.GetString();
                            int n = strlen(s1);
                            char *s2 = (char *)malloc(n + 1);
                            memcpy(s2, s1, n + 1);
                            g_array_append_val(header->channels, s2);
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
