#pragma once
#include "aieGlobal.h"
#include "SystemInfo.h"
#include "TestsFwd.h"
#include "opencv2/opencv.hpp"
#include <memory>

namespace aiutils {
    using namespace cv;
    using namespace std;

    class CacheData {
    private:
        shared_ptr<map<string, vector<unsigned char>>> _pData;
        unsigned int _cost = 0;
    public:
        explicit CacheData(const shared_ptr<map<string, vector<unsigned char>>>& pData = nullptr):_pData(pData) {
            if (_pData) {
                for(auto kv: *_pData) {
                    _cost += (kv.second.size() + kv.first.size())/1000;
                }
            }
        }

        unsigned int cost() const { return _cost; }
        shared_ptr<map<string, vector<unsigned char>>> get() const { return _pData; }
    };

    class AIE_EXPORT CacheManager {
        friend class test::Cache;
    private:
        // If ever used for many keys, consider using a linked hashmap, which makes everything amortized constant-time
        map<string, CacheData> _cache;
        vector<string> _recentlyUsed;
        unsigned int _maxCost = 0, _totalCost = 0;

        inline static CacheManager* __pCacheManager = nullptr;
    protected:
        explicit CacheManager(int maxCost): _maxCost(maxCost) {}

        // If key aliases something in _recentlyUsed, it may be modified.
        void recentlyUsed(const string& key) {
            auto recUsedIt = std::find(_recentlyUsed.begin(), _recentlyUsed.end(), key);
            if (recUsedIt != _recentlyUsed.end())
                // move recUsedIt to the end, preserving order elsewhere.
                std::rotate(recUsedIt, recUsedIt + 1, _recentlyUsed.end());
        }

        // If key aliases something in _recentlyUsed, it may be modified.
        shared_ptr<map<string, vector<unsigned char>>> fetch(const string& key) {
            auto cacheIt = _cache.find(key);
            if (cacheIt == _cache.end())
                return nullptr;
            recentlyUsed(key);
            return cacheIt->second.get();
        }

        // If key aliases something in _recentlyUsed or _cache, it may be modified or invalidated.
        bool remove(const string& key) {
            auto cacheIt = _cache.find(key);
            if (cacheIt == _cache.end())
                return false;
            _totalCost -= cacheIt->second.cost();
            auto recUsedIt = std::find(_recentlyUsed.begin(), _recentlyUsed.end(), key);
            _recentlyUsed.erase(recUsedIt);
            _cache.erase(cacheIt);
            return true;
        }

        // If data aliases something in _cache, but the key is new, it may be modified or invalidated.
        bool add(const string& key, const CacheData& data) {
            if (data.cost() > _maxCost) {
                _cache.clear();
                _recentlyUsed.clear();
                _totalCost = 0;
                return false;
            }

            auto [cacheIt, inserted] = _cache.try_emplace(key, data);
            if (!inserted)
                return false;
            _totalCost += data.cost();

            auto firstRetained = _recentlyUsed.begin();
            // Don't need to bounds check since we know data.cost() <= _maxCost
            for(; _totalCost > _maxCost; ++firstRetained) {
                auto erasedCacheIt = _cache.find(*firstRetained);
                _totalCost -= erasedCacheIt->second.cost();
                _cache.erase(erasedCacheIt);
            }
            _recentlyUsed.erase(_recentlyUsed.begin(), firstRetained);
            // cacheIt->first == key except key could have been invalidated
            _recentlyUsed.push_back(cacheIt->first);
            return true;
        }

        bool contains(const string& key) const {
            return _cache.find(key) != _cache.end();
        }

    public:
        static CacheManager& GetInstance(float percentage = 0.1f) {
            if (__pCacheManager == nullptr) {
                auto totalMemoryGB = SystemInfo::GetInstance().fetchMemory().first;
                __pCacheManager = new CacheManager(totalMemoryGB * (1 << 20) * percentage);
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
