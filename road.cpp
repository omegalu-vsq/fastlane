#include "road.hh"

#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>

namespace fast {

    Road::Road(float width, float& ref) : zref(ref) {
        road_texture = Texture::load("textures/basicroad.txt");
        road_texture.flip_vertical();
        grass_texture = Texture::load("textures/grass.ppm");

        sprite_zone = SpriteZone::load("textures/sunfield.txt");
        sprite_zone.setRoadOffset(0.15f);
        sprite_zone.setWidthScale(2.0f);

        this->width = width;
        this->grass_width = width;
        this->segment_length = strip_size / sprite_zone.height_;

        parseTrack("roads/track.txt");
    };

    void Road::setRoadTexture(const std::string& filename) {
        road_texture = Texture::load(filename);
        road_texture.flip_vertical();
    }

    void Road::setGrassTexture(const std::string& filename) {
        grass_texture = Texture::load(filename);
    }

    void Road::setSpriteZone(const std::string& filename) {
        float roadOffset = sprite_zone.roadOffset_;
        float widthScale = sprite_zone.widthScale_;
        sprite_zone = SpriteZone::load(filename);
        sprite_zone.setRoadOffset(roadOffset);
        sprite_zone.setWidthScale(widthScale);
        segment_length = strip_size / sprite_zone.height_;
    }

    float Road::catmullRomSpline(std::vector<float>& v, float t, int i, float tens) {
        i = i % v.size();
        int im1 = i - 1;
        if (im1 < 0) im1 = static_cast<int>(v.size()) - 1;
        int ip1 = (i + 1) % static_cast<int>(v.size());
        int ip2 = (i + 2) % static_cast<int>(v.size());

        return (v[i]
            + (-tens * v[im1] + tens * v[ip1]) * t
            + (2 * tens * v[im1] + (tens - 3) * v[i] + (3 - 2 * tens) * v[ip1] - tens * v[ip2]) * std::pow(t, 2)
            + (-tens * v[im1] + (2 - tens) * v[i] + (tens - 2) * v[ip1] + tens * v[ip2]) * std::pow(t, 3));
    }

    float Road::catmullRomSplineDerived(std::vector<float>& v, float t, int i, float tens) {
        i = i % v.size();
        int im1 = i - 1;
        if (im1 < 0) im1 = static_cast<int>(v.size()) - 1;
        int ip1 = (i + 1) % static_cast<int>(v.size());
        int ip2 = (i + 2) % static_cast<int>(v.size());

        return ((-tens * v[im1] + tens * v[ip1])
                        + (2 * tens * v[im1] + (tens - 3) * v[i] + (3 - 2 * tens) * v[ip1] - tens * v[ip2]) * 2 * t
                        + (-tens * v[im1] + (2 - tens) * v[i] + (tens - 2) * v[ip1] + tens * v[ip2]) * 3 * std::pow(t, 2));

    }

    float Road::trackXAt(float z) {
        if (centers.empty())
            return 0.0f;

        int idx = static_cast<int>(z / altitude_step);

        float t = std::fmod(z, altitude_step) / altitude_step;
        if (t < 0.0f) t += 1.0f;

        return curve_strength * catmullRomSpline(centers, t, idx, tension);
    }

    float Road::altitude(float z) {
        if (altitudes.empty())
            return 0.0f;

        int idx = static_cast<int>(z / altitude_step);

        float t = std::fmod(z, altitude_step) / altitude_step;
        if (t < 0.0f) t += 1.0f;

        return altitude_strength * catmullRomSpline(altitudes, t, idx, tension);
    }

    float Road::altitudeDerived(float z) {
        if (altitudes.empty())
            return 0.0f;

        int idx = static_cast<int>(z / altitude_step);

        float t = std::fmod(z, altitude_step) / altitude_step;
        if (t < 0.0f) t += 1.0f;

        return (altitude_strength * catmullRomSplineDerived(altitudes, t, idx, tension)) / altitude_step;
    }

    void Road::parseTrack(const std::string& filename) {
        altitudes.clear();
        centers.clear();

        std::ifstream file(filename);

        if (!file.is_open()) {
            throw std::runtime_error("/!\\ Road error: Track file openning failed.");
            return;
        }

        std::string line;
        while (std::getline(file, line)) {
            // Format:
            //   <altitude> <center offset>
            try {
                size_t start = 0;
                while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start])))
                    ++start;
                if (start >= line.size())
                    continue;
                if (line[start] == '#')
                    continue;

                size_t pos = start;
                float a = std::stof(line.substr(pos), &pos);
                altitudes.push_back(a);

                pos += start;
                while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos])))
                    ++pos;

                float second = 0.0f;
                if (pos < line.size())
                    second = std::stof(line.substr(pos));

                centers.push_back(second);
            } catch (const std::exception&) {
            }
        }

        if (altitudes.empty() || centers.empty()) {
            altitudes.assign(4, 0.0f);
            centers.assign(4, 0.0f);
        } else if (centers.size() != altitudes.size()) {
            centers.resize(altitudes.size(), 0.0f);
        }
    }
}
