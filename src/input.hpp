#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/CCTouchDispatcher.hpp>

#include <cstdint>
#include <unordered_set>

// Returns the current time in ms, relative to whatever the system uses for input events
// (usually system boot)
std::uint64_t platform_get_time();

class ExtendedCCEvent : public cocos2d::CCEvent {
    std::uint64_t m_timestamp;

public:
    ExtendedCCEvent(std::uint64_t timestamp) : m_timestamp(timestamp) {}

    std::uint64_t getTimestamp() const {
        return m_timestamp;
    }
};

// Touch dispatcher that adds a timestamp to events.
struct ExtendedCCTouchDispatcher : geode::Modify<ExtendedCCTouchDispatcher, cocos2d::CCTouchDispatcher> {
    static std::uint64_t g_lastTimestamp;
    static void setTimestamp(std::uint64_t);

    static void onModify(auto& self) {
        // Future proofing hook priority.
        (void)self.setHookPriority("cocos2d::CCTouchDispatcher::touches", -1000);
    }

    void touches(cocos2d::CCSet* pTouches, cocos2d::CCEvent* pEvent, unsigned int uIndex);
};