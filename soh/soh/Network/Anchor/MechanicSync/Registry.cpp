#include "Registry.h"

std::vector<MechanicFamily>& MechanicFamilyRegistry::All() {
    static std::vector<MechanicFamily> families;
    return families;
}

const MechanicFamily* MechanicFamilyRegistry::Find(s16 actorId) {
    for (const auto& f : All()) {
        if (f.actorId == actorId) {
            return &f;
        }
    }
    return nullptr;
}
