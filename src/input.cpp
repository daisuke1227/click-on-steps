#include "main.hpp"
#include "async.hpp"
#include "input.hpp"

#define DEBUG_STEPS false

#if DEBUG_STEPS
// This helper is for debugging pointer offsets.
template <typename T>
inline T* ptr_to_offset(void* base, unsigned int offset) {
    return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(base) + offset);
};

template <typename T>
inline T get_from_offset(void* base, unsigned int offset) {
    return *ptr_to_offset<T>(base, offset);
};
#endif

// Get timestamp with compatibility for custom keybinds.
// On macOS we use keyboard event info; for non-macOS, return 0.
std::uint64_t getTimestampCompat() {
#ifdef GEODE_IS_MACOS
    auto event = ExtendedCCKeyboardDispatcher::getCurrentEventInfo();
    if (!event) {
        return 0;
    }
    auto extendedInfo = static_cast<ExtendedCCEvent*>(event);
    return extendedInfo->getTimestamp();
#else
    return 0;
#endif
}

#ifdef GEODE_IS_WINDOWS
void CustomGJBaseGameLayer::queueButton_custom(int btnType, bool push, bool secondPlayer) {
#else
void CustomGJBaseGameLayer::queueButton(int btnType, bool push, bool secondPlayer) {
#endif
    // Workaround for passing arguments to methods.
    auto& fields = this->m_fields;

#ifdef GEODE_IS_ANDROID
    // If the player is currently in a frame, insert the input directly.
    if (fields->m_inFrame) {
        return GJBaseGameLayer::queueButton(btnType, push, secondPlayer);
    }
#endif

    auto inputTimestamp = static_cast<AsyncUILayer*>(this->m_uiLayer)->getLastTimestamp();
#ifdef GEODE_IS_MACOS
    if (!inputTimestamp) {
        inputTimestamp = getTimestampCompat();
    }
#endif

    auto timeRelativeBegin = fields->m_timeBeginMs;
    auto currentTime = inputTimestamp - timeRelativeBegin;
    if (inputTimestamp < timeRelativeBegin || !inputTimestamp || !timeRelativeBegin) {
        // When time isn't initialized yet, use 0.
        currentTime = 0;
    }

    auto inputOffset = fields->m_inputOffset;
    if (fields->m_inputOffsetRand != 0) {
        auto offsetMax = fields->m_inputOffsetRand;
        auto randValue = static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
        auto randOffset = static_cast<std::int32_t>((offsetMax * 2) * randValue - offsetMax);
        inputOffset += randOffset;
    }

    if (inputOffset < 0 && -inputOffset > currentTime) {
        currentTime = 0;
    } else {
        currentTime += inputOffset;
    }

#if DEBUG_STEPS
    geode::log::debug("queueing input type={} down={} p2={} at time {} (ts {} -> {})",
        btnType, push, secondPlayer, currentTime, timeRelativeBegin, inputTimestamp);
#endif

    fields->m_timedCommands.push({
        static_cast<PlayerButton>(btnType),
        push,
        secondPlayer,
        static_cast<std::int32_t>(currentTime)
    });
}

void CustomGJBaseGameLayer::resetLevelVariables() {
    GJBaseGameLayer::resetLevelVariables();

    auto& fields = m_fields;
    fields->m_timeBeginMs = 0;
    fields->m_timeOffset = 0.0;
    fields->m_timedCommands = {};
    fields->m_disableInputCutoff = geode::Mod::get()->getSettingValue<bool>("late-input-cutoff");
    fields->m_inputOffset = static_cast<std::int32_t>(
        geode::Mod::get()->getSettingValue<std::int64_t>("input-offset"));
    fields->m_inputOffsetRand = static_cast<std::int32_t>(
        geode::Mod::get()->getSettingValue<std::int64_t>("input-offset-rand"));
}

void CustomGJBaseGameLayer::processTimedInputs() {
    auto& fields = m_fields;
    auto timeMs = static_cast<std::uint64_t>(fields->m_timeOffset * 1000.0);
    auto& commands = fields->m_timedCommands;
    if (!commands.empty()) {
        auto nextTime = commands.top().m_step;

#if DEBUG_STEPS
        geode::log::debug("step info: time={}, waiting for {}", timeMs, nextTime);
#endif

        while (!commands.empty() && nextTime <= timeMs) {
            auto btn = commands.top();
            commands.pop();

#if DEBUG_STEPS
            geode::log::debug("queuedInput: btn={} push={} p2={} timeMs={}",
                static_cast<int>(btn.m_button), btn.m_isPush, btn.m_isPlayer2, btn.m_step);
#endif

#ifdef GEODE_IS_ANDROID
            queueButton(static_cast<int>(btn.m_button), btn.m_isPush, btn.m_isPlayer2);
#else
            this->m_queuedButtons.push_back(btn);
#endif

            if (!commands.empty()) {
                nextTime = commands.top().m_step;
            }
        }
    }
}

void CustomGJBaseGameLayer::updateInputQueue() {
    auto& fields = this->m_fields;
    auto& commands = fields->m_timedCommands;
    auto timeMs = static_cast<std::int32_t>(fields->m_timeOffset * 1000.0);
    PlayerButtonCommandQueue newCommands{};

    while (!commands.empty()) {
        auto btn = commands.top();
        commands.pop();
        btn.m_step = std::max(btn.m_step - timeMs, 0);
        newCommands.push(btn);
    }

    commands.swap(newCommands);
}

void CustomGJBaseGameLayer::dumpInputQueue() {
    auto& fields = this->m_fields;
    auto& commands = fields->m_timedCommands;

    while (!commands.empty()) {
        auto btn = commands.top();
        commands.pop();

#if DEBUG_STEPS
        geode::log::debug("failsafe queuedInput: btn={} push={} p2={} timeMs={}",
            static_cast<int>(btn.m_button), btn.m_isPush, btn.m_isPlayer2, btn.m_step);
#endif

#ifdef GEODE_IS_ANDROID
        queueButton(static_cast<int>(btn.m_button), btn.m_isPush, btn.m_isPlayer2);
#else
        this->m_queuedButtons.push_back(btn);
#endif
    }
}

void CustomGJBaseGameLayer::update(float dt) {
    auto& fields = m_fields;
    fields->m_timeBeginMs = platform_get_time();
    fields->m_timeOffset = 0.0;

    fields->m_inFrame = true;

    GJBaseGameLayer::update(dt);

    if (fields->m_disableInputCutoff) {
        updateInputQueue();
    } else {
        dumpInputQueue();
    }

    fields->m_inFrame = false;
}

void CustomGJBaseGameLayer::processCommands(float timeStep) {
    auto timeWarp = m_gameState.m_timeWarp;
    m_fields->m_timeOffset += timeStep / timeWarp;
    processTimedInputs();
    GJBaseGameLayer::processCommands(timeStep);
}

void CustomGJBaseGameLayer::fixUntimedInputs() {
#ifdef GEODE_IS_WINDOWS
    for (const auto& btn : this->m_queuedButtons) {
        this->queueButton_custom(static_cast<int>(btn.m_button), btn.m_isPush, btn.m_isPlayer2);
    }
    this->m_queuedButtons.clear();
#endif
}