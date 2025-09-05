#include "stdafx.h"
#include "MemoryPatcher.h"
#include "MessageHandler.h"

namespace IGCS
{
    // Static member definitions
    std::unordered_map<std::string, MemoryPatch> MemoryPatcher::_patches;
    LPBYTE MemoryPatcher::_moduleBase = nullptr;
    bool MemoryPatcher::_initialized = false;

    bool MemoryPatcher::initialize()
    {
        if (_initialized)
        {
            return true;
        }

        MODULEINFO moduleInfo = Utils::getModuleInfoOfContainingProcess();
        if (!moduleInfo.lpBaseOfDll)
        {
            MessageHandler::logError("MemoryPatcher: Failed to get module base address");
            return false;
        }

        _moduleBase = static_cast<LPBYTE>(moduleInfo.lpBaseOfDll);
        _patches.reserve(32); // Reserve space for efficiency
        _initialized = true;

        MessageHandler::logDebug("MemoryPatcher initialized with base address: %p", _moduleBase);
        return true;
    }

    void MemoryPatcher::cleanup()
    {
        if (_initialized && !_patches.empty())
        {
            toggleAllPatches(false);
            _patches.clear();
        }
        _initialized = false;
        _moduleBase = nullptr;
    }

    void MemoryPatcher::addPatch(const std::string& name, DWORD offset, std::initializer_list<uint8_t> bytes)
    {
        if (_patches.find(name) != _patches.end())
        {
            MessageHandler::logDebug("Patch '%s' already exists, overwriting", name.c_str());
        }

        _patches.emplace(name, MemoryPatch(name, offset, bytes));
        MessageHandler::logDebug("Added patch '%s' at offset 0x%X (%zu bytes)",
            name.c_str(), offset, bytes.size());
    }

    void MemoryPatcher::addPatch(const std::string& name, DWORD offset, const std::vector<uint8_t>& bytes)
    {
        if (_patches.find(name) != _patches.end())
        {
            MessageHandler::logDebug("Patch '%s' already exists, overwriting", name.c_str());
        }

        _patches.emplace(name, MemoryPatch(name, offset, bytes));
        MessageHandler::logDebug("Added patch '%s' at offset 0x%X (%zu bytes)",
            name.c_str(), offset, bytes.size());
    }

    void MemoryPatcher::addPatches(std::initializer_list<MemoryPatch> patches)
    {
        for (const auto& patch : patches)
        {
            addPatch(patch.name, patch.offset, patch.newBytes);
        }
    }

    bool MemoryPatcher::togglePatch(const std::string& name, bool enable)
    {
        if (!_initialized)
        {
            MessageHandler::logError("MemoryPatcher not initialized");
            return false;
        }

        auto it = _patches.find(name);
        if (it == _patches.end())
        {
            MessageHandler::logError("Patch '%s' not found", name.c_str());
            return false;
        }

        return applyPatch(it->second, enable);
    }

    void MemoryPatcher::toggleAllPatches(bool enable)
    {
        if (!_initialized)
        {
            MessageHandler::logError("MemoryPatcher not initialized");
            return;
        }

        int successCount = 0;
        for (auto& [name, patch] : _patches)
        {
            if (applyPatch(patch, enable))
            {
                successCount++;
            }
        }

        MessageHandler::logLine("Toggled %d/%zu patches to %s",
            successCount, _patches.size(), enable ? "enabled" : "disabled");
    }

    void MemoryPatcher::togglePatchGroup(const std::vector<std::string>& names, bool enable)
    {
        if (!_initialized)
        {
            MessageHandler::logError("MemoryPatcher not initialized");
            return;
        }

        int successCount = 0;
        for (const auto& name : names)
        {
            auto it = _patches.find(name);
            if (it != _patches.end() && applyPatch(it->second, enable))
            {
                successCount++;
            }
        }

        MessageHandler::logLine("Patch group: %d/%zu patches %s",
            successCount, names.size(), enable ? "enabled" : "disabled");
    }

    bool MemoryPatcher::isPatchActive(const std::string& name)
    {
        auto it = _patches.find(name);
        return (it != _patches.end()) ? it->second.isPatched : false;
    }

    bool MemoryPatcher::getPatchInfo(const std::string& name, MemoryPatch& outPatch)
    {
        auto it = _patches.find(name);
        if (it == _patches.end())
        {
            return false;
        }
        outPatch = it->second;
        return true;
    }

    void MemoryPatcher::removePatch(const std::string& name)
    {
        auto it = _patches.find(name);
        if (it != _patches.end())
        {
            if (it->second.isPatched)
            {
                applyPatch(it->second, false);
            }
            _patches.erase(it);
            MessageHandler::logDebug("Removed patch '%s'", name.c_str());
        }
    }

    void MemoryPatcher::clearAllPatches()
    {
        if (!_patches.empty())
        {
            toggleAllPatches(false);
            _patches.clear();
        }
        MessageHandler::logDebug("Cleared all patches");
    }

    std::vector<std::string> MemoryPatcher::getAllPatchNames()
    {
        std::vector<std::string> names;
        names.reserve(_patches.size());

        for (const auto& [name, patch] : _patches)
        {
            names.push_back(name);
        }

        return names;
    }

    bool MemoryPatcher::applyPatch(MemoryPatch& patch, bool enable)
    {
        LPBYTE targetAddress = calculateAddress(patch.offset);
        if (!targetAddress)
        {
            MessageHandler::logError("Invalid address for patch '%s'", patch.name.c_str());
            return false;
        }

        const size_t patchSize = patch.newBytes.size();

        if (patch.isPatched == enable)
        {
            return true;
        }

        if (enable && patch.originalBytes.empty())
        {
            patch.originalBytes.resize(patchSize);
            GameImageHooker::readRange(targetAddress, patch.originalBytes.data(),
                static_cast<int>(patchSize));

            MessageHandler::logDebug("Saved %zu original bytes for patch '%s'",
                patchSize, patch.name.c_str());
        }

        if (enable)
        {
            GameImageHooker::writeRange(targetAddress, patch.newBytes.data(),
                static_cast<int>(patchSize));
            patch.isPatched = true;
            MessageHandler::logDebug("Applied patch '%s' at address %p",
                patch.name.c_str(), targetAddress);
        }
        else if (!patch.originalBytes.empty())
        {
            GameImageHooker::writeRange(targetAddress, patch.originalBytes.data(),
                static_cast<int>(patchSize));
            patch.isPatched = false;
            MessageHandler::logDebug("Restored patch '%s' at address %p",
                patch.name.c_str(), targetAddress);
        }
        else
        {
            MessageHandler::logDebug("Cannot restore patch '%s' - no original bytes saved",
                patch.name.c_str());
            return false;
        }

        return true;
    }
}