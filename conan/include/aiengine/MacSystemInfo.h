#ifndef MACSYSTEMINFO_H
#define MACSYSTEMINFO_H
#include "DeviceInfo.h"
#include <vector>

extern int __appClosing;

void mac_appStarting();
void mac_appClosing();
float mac_version();
bool mac_isAppTranslated();
std::vector<aiutils::DeviceInfo> mac_gpuList();
std::string mac_machineId();
bool mac_isLowPowerMode();

#endif // CMSYSTEMINFO_H
