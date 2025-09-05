#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "Utils.h"
#include "GameImageHooker.h"

namespace IGCS
{
    struct MemoryPatch
    {
        std::string name;
        DWORD offset;           // Offset from module base
        std::vector<uint8_t> newBytes;
        std::vector<uint8_t> originalBytes;
        bool isPatched;

        MemoryPatch() : offset(0), isPatched(false) {}

        MemoryPatch(const std::string& patchName, DWORD patchOffset, std::initializer_list<uint8_t> bytes)
            : name(patchName), offset(patchOffset), newBytes(bytes), isPatched(false) {
        }

        MemoryPatch(const std::string& patchName, DWORD patchOffset, const std::vector<uint8_t>& bytes)
            : name(patchName), offset(patchOffset), newBytes(bytes), isPatched(false) {
        }

        template<size_t N>
        MemoryPatch(const std::string& patchName, DWORD patchOffset, const uint8_t(&bytes)[N])
            : name(patchName), offset(patchOffset), newBytes(bytes, bytes + N), isPatched(false) {
        }
    };

    class MemoryPatcher
    {
    private:
        // Prevent instantiation
        MemoryPatcher() = delete;
        ~MemoryPatcher() = delete;

        // Static member variables
        static std::unordered_map<std::string, MemoryPatch> _patches;
        static LPBYTE _moduleBase;
        static bool _initialized;

    public:
        // Initialize the patcher with the module base address
        static bool initialize();

        // Cleanup - call this before program exit
        static void cleanup();

        // Add a patch definition with initializer_list
        static void addPatch(const std::string& name, DWORD offset, std::initializer_list<uint8_t> bytes);

        // Add a patch definition with vector
        static void addPatch(const std::string& name, DWORD offset, const std::vector<uint8_t>& bytes);

        // Add multiple patches at once
        static void addPatches(std::initializer_list<MemoryPatch> patches);

        // Enable/disable a specific patch by name
        static bool togglePatch(const std::string& name, bool enable);

        // Enable/disable all patches
        static void toggleAllPatches(bool enable);

        // Enable/disable a group of patches
        static void togglePatchGroup(const std::vector<std::string>& names, bool enable);

        // Check if a patch is currently active
        static bool isPatchActive(const std::string& name);

        // Get patch info
        static bool getPatchInfo(const std::string& name, MemoryPatch& outPatch);

        // Remove a patch
        static void removePatch(const std::string& name);

        // Clear all patches
        static void clearAllPatches();

        // Get all patch names
        static std::vector<std::string> getAllPatchNames();

    private:
        static bool applyPatch(MemoryPatch& patch, bool enable);
        static inline LPBYTE calculateAddress(DWORD offset) { return _moduleBase ? _moduleBase + offset : nullptr; }
    };
}