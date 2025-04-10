#import <UIKit/UIKit.h>
#include <objc/runtime.h>
#include <Geode/Geode.hpp>
#include <cstdint>
#include <time.h>
#include "input.hpp"

// Returns the system uptime in milliseconds.
std::uint64_t platform_get_time() {
    // convert ns to ms
    return clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1'000'000;
}

// Define EAGLView as a subclass of UIView for iOS.
@interface EAGLView : UIView
@end

#ifdef GEODE_IS_IOS
// Function pointers for original method implementations.
static IMP touchesBeganOIMP;
static IMP touchesMovedOIMP;
static IMP touchesEndedOIMP;
static IMP touchesCancelledOIMP;

// Overriding touchesBegan:withEvent:
void touchesBegan(EAGLView* self, SEL sel, NSSet* touches, UIEvent* event) {
    auto timestamp = static_cast<std::uint64_t>([event timestamp] * 1000.0);
    ExtendedCCTouchDispatcher::setTimestamp(timestamp);
    reinterpret_cast<decltype(&touchesBegan)>(touchesBeganOIMP)(self, sel, touches, event);
}

// Overriding touchesMoved:withEvent:
void touchesMoved(EAGLView* self, SEL sel, NSSet* touches, UIEvent* event) {
    auto timestamp = static_cast<std::uint64_t>([event timestamp] * 1000.0);
    ExtendedCCTouchDispatcher::setTimestamp(timestamp);
    reinterpret_cast<decltype(&touchesMoved)>(touchesMovedOIMP)(self, sel, touches, event);
}

// Overriding touchesEnded:withEvent:
void touchesEnded(EAGLView* self, SEL sel, NSSet* touches, UIEvent* event) {
    auto timestamp = static_cast<std::uint64_t>([event timestamp] * 1000.0);
    ExtendedCCTouchDispatcher::setTimestamp(timestamp);
    reinterpret_cast<decltype(&touchesEnded)>(touchesEndedOIMP)(self, sel, touches, event);
}

// Overriding touchesCancelled:withEvent:
void touchesCancelled(EAGLView* self, SEL sel, NSSet* touches, UIEvent* event) {
    auto timestamp = static_cast<std::uint64_t>([event timestamp] * 1000.0);
    ExtendedCCTouchDispatcher::setTimestamp(timestamp);
    reinterpret_cast<decltype(&touchesCancelled)>(touchesCancelledOIMP)(self, sel, touches, event);
}
#endif

$execute {
    auto eaglView = objc_getClass("EAGLView");

#ifdef GEODE_IS_IOS
    // Hook the touch methods for iOS.
    auto touchesBeganMethod = class_getInstanceMethod(eaglView, @selector(touchesBegan:withEvent:));
    touchesBeganOIMP = method_getImplementation(touchesBeganMethod);
    method_setImplementation(touchesBeganMethod, (IMP)&touchesBegan);

    auto touchesMovedMethod = class_getInstanceMethod(eaglView, @selector(touchesMoved:withEvent:));
    touchesMovedOIMP = method_getImplementation(touchesMovedMethod);
    method_setImplementation(touchesMovedMethod, (IMP)&touchesMoved);

    auto touchesEndedMethod = class_getInstanceMethod(eaglView, @selector(touchesEnded:withEvent:));
    touchesEndedOIMP = method_getImplementation(touchesEndedMethod);
    method_setImplementation(touchesEndedMethod, (IMP)&touchesEnded);

    auto touchesCancelledMethod = class_getInstanceMethod(eaglView, @selector(touchesCancelled:withEvent:));
    touchesCancelledOIMP = method_getImplementation(touchesCancelledMethod);
    method_setImplementation(touchesCancelledMethod, (IMP)&touchesCancelled);
#endif
}