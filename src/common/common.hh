#pragma once

#include <vector>
#include <functional>
#include <cstdio>
#include <string>

class CPed;
class gameSkeleton;

namespace Rainbomizer {
class Common
{
    static inline bool mHashesInitialised = false;

    static inline std::vector<int> mVehicleHashes;
    static inline std::vector<int> mPedHashes;

    static void InitialiseHashes ();

public:
    static const std::vector<int> &GetVehicleHashes ();
    static const std::vector<int> &GetPedHashes ();

    static std::string GetRainbomizerFileName (const std::string &name,
                                               const std::string &subdirs = "");

    static FILE *GetRainbomizerFile (const std::string &name,
                                     const std::string &mode,
                                     const std::string &subdirs = "");

    static FILE *GetRainbomizerDataFile (const std::string &name,
                                         const std::string &mode = "r");
};
} // namespace Rainbomizer
