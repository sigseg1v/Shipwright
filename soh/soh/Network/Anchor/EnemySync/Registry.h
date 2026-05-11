#ifndef NETWORK_ANCHOR_ENEMYSYNC_REGISTRY_H
#define NETWORK_ANCHOR_ENEMYSYNC_REGISTRY_H

#include "Family.h"
#include <vector>

// Single global table of every EnemyFamily registered at startup. Family
// .cpp files under EnemySync/Families/ each push one entry via static
// initializer (see ANCHOR_REGISTER_ENEMY_FAMILY below). EnemySync.cpp
// then dispatches every per-actor operation through Find(actorId)
// instead of growing a switch statement for each new family.
class EnemyFamilyRegistry {
  public:
    static std::vector<EnemyFamily>& All();
    static const EnemyFamily* Find(s16 actorId);
};

// Helper used at file scope inside each Families/<Name>.cpp. Each
// translation unit declares one anonymous-namespace static whose
// initializer runs at program startup and pushes the family entry
// into the global registry. The anonymous namespace per .cpp file
// gives each registration its own name without colliding across
// translation units.
#define ANCHOR_REGISTER_ENEMY_FAMILY(family)                                                       \
    namespace {                                                                                    \
    [[maybe_unused]] const bool _anchor_enemy_family_registered = []() {                           \
        EnemyFamilyRegistry::All().push_back(family);                                              \
        return true;                                                                               \
    }();                                                                                           \
    }

#endif // NETWORK_ANCHOR_ENEMYSYNC_REGISTRY_H
