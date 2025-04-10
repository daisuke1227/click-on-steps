#include "async.hpp"
#include "main.hpp"

#define DEBUG_PROCESSING false

void AsyncUILayer::handleKeypress(cocos2d::enumKeyCodes key, bool down) {
    // Remove keyboard-specific code and directly call the base implementation.
    UILayer::handleKeypress(key, down);
}

bool AsyncUILayer::ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    if (!event) {
        return UILayer::ccTouchBegan(touch, event);
    }

    auto extendedInfo = static_cast<ExtendedCCEvent*>(event);
    m_fields->m_lastTimestamp = extendedInfo->getTimestamp();

    auto r = UILayer::ccTouchBegan(touch, event);
    static_cast<CustomGJBaseGameLayer*>(this->m_gameLayer)->fixUntimedInputs();

    m_fields->m_lastTimestamp = 0ull;
    return r;
}

void AsyncUILayer::ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    if (!event) {
#ifdef GEODE_IS_ANDROID
        if (this->m_inPlatformer) {
            this->processUINodesTouch(static_cast<GJUITouchEvent>(1), touch);
        }
        return;
#else
        return UILayer::ccTouchMoved(touch, event);
#endif
    }

    auto extendedInfo = static_cast<ExtendedCCEvent*>(event);
    m_fields->m_lastTimestamp = extendedInfo->getTimestamp();

#ifdef GEODE_IS_ANDROID
    if (this->m_inPlatformer) {
        this->processUINodesTouch(static_cast<GJUITouchEvent>(1), touch);
    }
#else
    UILayer::ccTouchMoved(touch, event);
    static_cast<CustomGJBaseGameLayer*>(this->m_gameLayer)->fixUntimedInputs();
#endif

    m_fields->m_lastTimestamp = 0ull;
}

void AsyncUILayer::ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    if (!event) {
        return UILayer::ccTouchEnded(touch, event);
    }

    auto extendedInfo = static_cast<ExtendedCCEvent*>(event);
    m_fields->m_lastTimestamp = extendedInfo->getTimestamp();

    UILayer::ccTouchEnded(touch, event);
    static_cast<CustomGJBaseGameLayer*>(this->m_gameLayer)->fixUntimedInputs();

    m_fields->m_lastTimestamp = 0ull;
}

#ifndef GEODE_IS_WINDOWS
void AsyncUILayer::ccTouchCancelled(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) {
    if (!event) {
        return this->ccTouchEnded(touch, event);
    }

    auto extendedInfo = static_cast<ExtendedCCEvent*>(event);
    m_fields->m_lastTimestamp = extendedInfo->getTimestamp();

    this->ccTouchEnded(touch, event);
    m_fields->m_lastTimestamp = 0ull;
}
#endif

std::uint64_t AsyncUILayer::getLastTimestamp() {
    return m_fields->m_lastTimestamp;
}