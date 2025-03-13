#include "core/system.h"
#include "core/file_sys/registered_cache.h"
#include "core/file_sys/content_archive.h"
#include "core/crypto/key_manager.h"

bool ContentManager::IsFirmwareAvailable() {
    constexpr u64 MiiEditId = 0x0100000000001009; // Mii Edit applet ID
    constexpr u64 QLaunchId = 0x0100000000001000; // Home Menu applet ID

    auto& system = Core::System::GetInstance();
    auto bis_system = system.GetFileSystemController().GetSystemNANDContents();
    if (!bis_system) {
        return false;
    }

    auto mii_applet_nca = bis_system->GetEntry(MiiEditId, FileSys::ContentRecordType::Program);
    auto qlaunch_nca = bis_system->GetEntry(QLaunchId, FileSys::ContentRecordType::Program);

    if (!mii_applet_nca || !qlaunch_nca) {
        return false;
    }

    // Also check for essential keys
    return Core::Crypto::KeyManager::Instance().IsFirmwareAvailable();
}