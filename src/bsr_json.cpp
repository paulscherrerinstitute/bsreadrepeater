/*
Project: bsreadrepeater
License: GNU General Public License v3.0
Authors: Dominik Werder <dominik.werder@gmail.com>
*/

#include <bsr_json.h>
#include <rapidjson/document.h>
#include <stdio.h>

extern "C" ERRT json_parse(char *str, int n, GString *log) {
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
