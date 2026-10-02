#pragma once
#include "vector.h"
#include <cmath>

namespace mod {
inline bool VehicleDestination(CMatrix4f& vehicle,CMatrix4f& rider,const CVector3f& destination) {
    for(const auto* matrix:{&vehicle,&rider})
        for(const auto& row:matrix->m)
            if(!std::isfinite(row.x)||!std::isfinite(row.y)||!std::isfinite(row.z)||!std::isfinite(row.w)) return false;
    if(!std::isfinite(destination.x)||!std::isfinite(destination.y)||!std::isfinite(destination.z)) return false;
    rider.m[3].x+=destination.x-vehicle.m[3].x;
    rider.m[3].y+=destination.y-vehicle.m[3].y;
    rider.m[3].z+=destination.z-vehicle.m[3].z;
    vehicle.m[3]={destination.x,destination.y,destination.z,1};
    return true;
}
}
