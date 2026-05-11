#include "Registry.h"

std::vector<EnemyFamily>& EnemyFamilyRegistry::All() {
    static std::vector<EnemyFamily> families;
    return families;
}

const EnemyFamily* EnemyFamilyRegistry::Find(s16 actorId) {
    for (const auto& f : All()) {
        if (f.actorId == actorId) {
            return &f;
        }
    }
    return nullptr;
}
