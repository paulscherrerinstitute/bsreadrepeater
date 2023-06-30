#include <bsr_json_commands.h>
#include <rapidjson/document.h>
#include <stdio.h>
#include <string>

static ERRT extract_command(rapidjson::Document &doc, struct bsr_json_command *cmd) noexcept {
    using std::string;
    if (!doc.HasMember("kind")) {
        return -1;
    } else {
        auto &kind = doc["kind"];
        if (kind.IsString()) {
            auto kind_str = kind.GetString();
            if (string("add-source") == kind_str) {
                if (doc.HasMember("source")) {
                    if (!doc["source"].IsString()) {
                        return -1;
                    }
                } else {
                    return -1;
                }
                cmd->inner.add_source.msgmax = DEFAULT_MSG_MAX;
                cmd->inner.add_source.rcvhwm = DEFAULT_HWM;
                cmd->inner.add_source.rcvbuf = DEFAULT_RCVBUF;
                if (doc.HasMember("msgmax")) {
                    if (doc["msgmax"].IsInt()) {
                        cmd->inner.add_source.msgmax = doc["msgmax"].GetInt();
                    }
                }
                if (doc.HasMember("rcvhwm")) {
                    if (doc["rcvhwm"].IsInt()) {
                        cmd->inner.add_source.rcvhwm = doc["rcvhwm"].GetInt();
                    }
                }
                if (doc.HasMember("rcvbuf")) {
                    if (doc["rcvbuf"].IsInt()) {
                        cmd->inner.add_source.rcvbuf = doc["rcvbuf"].GetInt();
                    }
                }
                cmd->kind = ADD_SOURCE;
                strncpy(cmd->inner.add_source.source, doc["source"].GetString(), SOURCE_ADDR_MAX);
            } else if (string("add-output") == kind_str) {
                cmd->kind = ADD_OUTPUT;
                if (!doc.HasMember("source")) {
                    return -1;
                }
                if (!doc["source"].IsString()) {
                    return -1;
                }
                if (!doc.HasMember("output")) {
                    return -1;
                }
                if (!doc["output"].IsString()) {
                    return -1;
                }
                strncpy(cmd->inner.add_output.source, doc["source"].GetString(), SOURCE_ADDR_MAX);
                strncpy(cmd->inner.add_output.output, doc["output"].GetString(), SOURCE_ADDR_MAX);
                cmd->inner.add_output.sndhwm = DEFAULT_HWM;
                cmd->inner.add_output.sndbuf = DEFAULT_SNDBUF;
                if (doc.HasMember("sndhwm")) {
                    if (doc["sndhwm"].IsInt()) {
                        cmd->inner.add_output.sndhwm = doc["sndhwm"].GetInt();
                    }
                }
                if (doc.HasMember("sndbuf")) {
                    if (doc["sndbuf"].IsInt()) {
                        cmd->inner.add_output.sndbuf = doc["sndbuf"].GetInt();
                    }
                }
            } else if (string("remove-source") == kind_str) {
                // TODO
            } else if (string("exit") == kind_str) {
            } else if (string("dummy-source-start") == kind_str) {
                cmd->kind = DUMMY_SOURCE_START;
            } else {
                return -1;
            }
        }
        return 0;
    }
}

extern "C" ERRT parse_json_command(char const *str, int n, struct bsr_json_command *cmd) {
    using rapidjson::Value;
    rapidjson::Document doc;
    try {
        doc.Parse(str, n);
        if (doc.HasParseError()) {
            fprintf(stderr, "ERROR parse_json_command parser error\n");
            return -1;
        } else {
            return extract_command(doc, cmd);
        }
    } catch (std::exception const &e) {
        fprintf(stderr, "ERROR parse_json_command exception\n");
        return -1;
    }
    return 0;
}
