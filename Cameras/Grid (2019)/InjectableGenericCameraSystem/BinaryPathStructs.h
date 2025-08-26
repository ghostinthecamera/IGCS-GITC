#pragma once
#include <cstdint>

#include "PathManager.h"

namespace IGCS::BinaryPathStructs
{
    // Binary format version - increment when structure changes
    constexpr uint8_t BINARY_PATH_FORMAT_VERSION = 1;

    // Binary structures must be packed to ensure consistent memory layout
#pragma pack(push, 1)

// Header for the entire data packet
    struct BinaryPathHeader {
        uint8_t formatVersion;     // Format version for compatibility checking
        uint8_t pathCount;         // Number of paths in the packet
    };

    // Header for each path
    struct BinaryPathData {
        uint16_t nameLength;       // Length of the path name string (keep uint16_t for longer names)
        uint8_t nodeCount;         // Number of nodes in this path
        // Followed by nameLength bytes for the path name (char array)
        // Followed by nodeCount BinaryNodeData structures
    };

    // Data for a single camera node
    struct BinaryNodeData {
        uint8_t index;             // Node index
        float position[3];         // x, y, z
        float rotation[4];         // x, y, z, w (quaternion)
        float fov;                 // Field of view
    };

#pragma pack(pop)

    // Helper function to calculate the size of a path in bytes
    inline size_t calculatePathSize(const std::string& pathName, size_t nodeCount) {
        return sizeof(BinaryPathData) + pathName.length() + (nodeCount * sizeof(BinaryNodeData));
    }

    // Helper function to calculate the total size of all paths in bytes
    inline size_t calculateTotalSize(const PathManager& manager) {
        size_t totalSize = sizeof(BinaryPathHeader);

        for (const auto& pair : manager.getPaths()) {
            const std::string& pathName = pair.first;
            const CameraPath& path = pair.second;
            totalSize += calculatePathSize(pathName, path.GetNodeCount());
        }

        return totalSize;
    }
}
