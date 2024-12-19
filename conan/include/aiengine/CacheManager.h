#pragma once
#include "aieGlobal.h"
#include "SystemInfo.h"
#include "opencv2/opencv.hpp"
#include <memory>

namespace aiutils {
    using namespace cv;
    using namespace std;

    class CacheData {
        shared_ptr<map<string, vector<unsigned char>>> _pData;
        unsigned int _cost;
    public:
        CacheData(const shared_ptr<map<string, vector<unsigned char>>>& pData = nullptr):_pData(pData) {
            _cost = 0;
            if(_pData) {
                for(auto kv: *_pData) {
                    _cost += (kv.second.size() + kv.first.size())/1000;
                }
            }
        }

        unsigned int cost() const { return _cost; }
        shared_ptr<map<string, vector<unsigned char>>> get() const { return _pData; }
    };

    class AIE_EXPORT CacheManager {
        map<string, CacheData> _cache;
        vector<string> _recentlyUsed;
        unsigned int _maxCost, _totalCost;

        inline static CacheManager* __pCacheManager = nullptr;
    protected:
        CacheManager(int maxCost):_maxCost(maxCost) {

        }

        void recentlyUsed(const string& key) {
            std::remove(_recentlyUsed.begin(), _recentlyUsed.end(), key);
            _recentlyUsed.push_back(key);
        }

        shared_ptr<map<string, vector<unsigned char>>> fetch(const string& key) {
            if(contains(key)) {
                recentlyUsed(key);
                return _cache[key].get();
            }
            return nullptr;
        }

        bool remove(const string& key) {
            if(_cache.find(key) != _cache.end()) {
                _totalCost -= _cache[key].cost();
                _cache.erase(key);
                std::remove(_recentlyUsed.begin(), _recentlyUsed.end(), key);
                return true;
            }
            return false;
        }

        bool add(const string& key, const CacheData& data) {
            if(!contains(key)) {
                while(_maxCost < _totalCost + data.cost() && !_recentlyUsed.empty()) {
                    auto k = _recentlyUsed[0];
                    _recentlyUsed.pop_back();
                    remove(k);
                }
                if(_maxCost < data.cost())
                    return false;
                _totalCost += data.cost();
                _cache[key] = data;
                _recentlyUsed.push_back(key);
                return true;
            }
            return false;
        }

        bool contains(const string& key) {
            return _cache.find(key) != _cache.end();
        }

    public:
        static CacheManager& GetInstance(float percentage = 0.1f) {
            if(__pCacheManager == nullptr) {
                auto totalMemoryGB = SystemInfo::GetInstance().fetchMemory().first;
                __pCacheManager = new CacheManager(totalMemoryGB*1000000*percentage);
            }
            return *__pCacheManager;
        }

        static bool Contains(const string& key) {
            return GetInstance().contains(key);
        }

        static bool Remove(const string& key) {
            return GetInstance().remove(key);
        }

        static void LogStats() {
            GetInstance().logStats();
        }

        static shared_ptr<map<string, vector<unsigned char>>> Fetch(const string& key) {
            return GetInstance().fetch(key);
        }

        static bool AddModel(const string& key, const shared_ptr<map<string, vector<unsigned char>>>& pBuffer) {
            return GetInstance().add(key, CacheData(pBuffer));
        }

        void logStats() {
            AIE_INFO() << " Cache: Max " << _maxCost << " Total " << _totalCost <<
                    "Avaliable " << _maxCost - _totalCost  << " Count " << _cache.size();
        }
    };

}
