#include "anim_mesh.h"
#include "texture_to_render.h"
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <glm/gtx/io.hpp>
#include <unordered_map>
#include <sys/stat.h>

/*
 * We put these functions to a separated file because the following jumbo
 * header consists of 20K LOC, and it is very slow to compile.
 */
#include "json.hpp"

using json = nlohmann::json;

namespace {
    std::string thumbnail_path_dir = ".thumbnails";
    std::string thumbnail_path_head = ".th";
    std::string thumbnail_path_tail = ".jpg";
}

void MeshI::saveAnimFile(const std::string &fn) {
    std::vector<json> conv_data_key_frames;
    std::transform(this->key_frames.begin(), this->key_frames.end(), std::back_inserter(conv_data_key_frames),
        [](const KeyFrame& c) {
            json j;
            j["tran"] = json::object();
            for (auto it : c.root_tran) {
                j["tran"][std::to_string(it.first)] = json::array({it.second[0], it.second[1], it.second[2]});
            }
            std::vector<json> conv_data_rota;
            std::transform(c.rori.begin(), c.rori.end(), std::back_inserter(conv_data_rota),
                           [](glm::fquat rota) { return json::array({rota[0], rota[1], rota[2], rota[3]}); }
            );
            j["rota"] = json(conv_data_rota);
            j["time"] = json(c.t.count());
            std::vector<unsigned char> vec;
            int w; int h;
            c.ttr->saveToBuf(w, h, vec);
            j["thw"] = w;
            j["thh"] = h;
            for (size_t i = 0; i < vec.size(); i++) j["th"][i] = vec[i];
            return j;
        }
    );
    json j;
    j["key_frames"] = (conv_data_key_frames);
    std::ofstream(fn) << j.dump(0) << std::endl;
}

void MeshI::loadAnimFile(const std::string &fn) {
    this->key_frames.clear();
    json j;
    std::ifstream(fn) >> j;
    std::vector<json> data_key_frames = j["key_frames"];
    std::transform(data_key_frames.begin(), data_key_frames.end(), std::back_inserter(this->key_frames),
        [](const json& j) {
            KeyFrame c;
            json data_tran = j["tran"];
            for (auto it = data_tran.begin(); it != data_tran.end(); ++it) {
                int k = std::stoi(it.key());
                std::vector<float> v = it.value();
                c.root_tran[k] = glm::vec3({v[0], v[1], v[2]});
            }
            std::vector<json> data_rota = j["rota"];
            std::transform(
                data_rota.begin(), data_rota.end(),
                std::back_inserter(c.rori),
                [](const json& rota) { return glm::fquat(rota[3], rota[0], rota[1], rota[2]); }
            );
            c.t = std::chrono::duration<double>(double(j["time"]));
            std::vector<unsigned char> data = j["th"];
            c.ttr = new TextureToRender(j["thw"], j["thh"], data.data());
            return c;
        }
    );
}
