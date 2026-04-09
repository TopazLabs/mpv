//
// Created by Gregory on 12/19/2024.
//

#ifndef RLMHANDLER_H
#define RLMHANDLER_H

#include "license.h"
#include <string>
#include "topaz_rlm_export.h"

#define TOPAZ_DEFAULT_ROAMING "<LICENSE topazlabs rlm_roam 1.0 permanent uncounted hostid=ANY\
  max_roam=7 _ck=821e039afd sig=\"60P04509SMY4H6G0G43BS16PGJ1JWCFUE1WDC\
  M822G5M29SQC9AFBMEWS1D8RJ1H8VG19JARY8\">"

class TOPAZ_RLM_EXPORT RlmHandler {
private:
    RLM_HANDLE _rlmHandle;
    RLM_LICENSE _rlmLicense;
public:
    RlmHandler(const std::string& licenseFile, const std::string& executablePath, const std::string& licenseString);
    ~RlmHandler();
    int checkout(const std::string& product, const std::string& version);
    void checkin();
    std::string getLicenseOptions() const;
    int getMaxRoamingTime() const;
    std::pair<std::string, int> getLicenseExpiration() const;
    std::string getLicenseVersion() const;
    std::string getCustomer() const;
    bool isInitialized() const;
    int heartbeat(bool queueing);
    bool isLicenseCached() const;
    bool isLicenseRoaming() const;
};



#endif //RLMHANDLER_H
