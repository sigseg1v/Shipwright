#ifndef NETWORK_ANCHOR_MECHANICSYNC_REGISTRY_H
#define NETWORK_ANCHOR_MECHANICSYNC_REGISTRY_H

#include "Family.h"
#include <vector>

// Mirror of EnemyFamilyRegistry. One global table of every
// MechanicFamily registered at startup; the tick code in
// MechanicSync.cpp looks up entries via Find(actorId) rather than
// growing a per-actor switch.
class MechanicFamilyRegistry {
  public:
    static std::vector<MechanicFamily>& All();
    static const MechanicFamily* Find(s16 actorId);
};

// Used at file scope inside each Families/<Name>.cpp. Each translation
// unit declares one anonymous-namespace static whose initializer runs
// at program startup and pushes the family entry into the global
// registry.
#define ANCHOR_REGISTER_MECHANIC_FAMILY(family)                                                    \
    namespace {                                                                                    \
    [[maybe_unused]] const bool _anchor_mechanic_family_registered = []() {                        \
        MechanicFamilyRegistry::All().push_back(family);                                           \
        return true;                                                                               \
    }();                                                                                           \
    }

#endif // NETWORK_ANCHOR_MECHANICSYNC_REGISTRY_H
