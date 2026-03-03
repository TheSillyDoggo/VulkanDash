#pragma once

#include <Geode/Geode.hpp>
#include "../vkbind.h"
#include "../VulkanRenderer.hpp"

using namespace geode::prelude;

class VKShader
{
    public:
        static std::vector<uint32_t> create(std::string path2, std::string path3)
        {
            std::string path = (Mod::get()->getResourcesDir() / utils::string::replace(path2, ""_spr, "")).string();

            if (!std::filesystem::exists(path))
            {
                std::string exe = (Mod::get()->getResourcesDir() / "glslang.exe").string();
                std::string vert = (Mod::get()->getResourcesDir() / utils::string::replace(path3, ""_spr, "")).string();
                std::string out  = path;

                std::string cmd2 = fmt::format("\"{}\" -V \"{}\" -o \"{}\"", exe, vert, out);
                std::string cmd = fmt::format("cmd /C \"{}\"", cmd2);

                system(cmd.c_str());
            }

            std::ifstream file(path, std::ios::binary | std::ios::ate);
            size_t size = file.tellg();
            file.seekg(0);
            std::vector<uint32_t> buffer(size / sizeof(uint32_t));
            file.read(reinterpret_cast<char*>(buffer.data()), size);
            return buffer;
        }
};