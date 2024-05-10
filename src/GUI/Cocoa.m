#ifdef USE_COCOA

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <BetterSpades/common.h>
#include <BetterSpades/main.h>
#include <BetterSpades/window.h>
#include <BetterSpades/config.h>
#include <BetterSpades/hud.h>

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>

#ifdef USE_GNUSTEP
    #import <GNUstepGUI/GSDisplayServer.h>
#endif

@interface GameView : NSOpenGLView
{
    BOOL _isNewerThanLion;
    BOOL _isMouseCaptured;
}

- (NSRect) frameOnScreen;
- (void) updateMouse;

- (void) captureMouse;
- (void) releaseMouse;

- (BOOL) isMouseCaptured;
@end

@implementation GameView
- (id) init
{
    _isMouseCaptured = NO;
    return [super init];
}

- (BOOL) acceptsFirstResponder
{
    return YES;
}

- (void) viewDidMoveToWindow
{
    [super viewDidMoveToWindow];

    _isNewerThanLion = [[self window] respondsToSelector:@selector(convertRectToScreen:)];
}

- (NSRect) frameOnScreen
{
    NSRect rect = [self frame];

    // https://trac.macports.org/ticket/52210?cversion=1&cnum_hist=57

    #if MAC_OS_X_VERSION_MAX_ALLOWED < 1070
        rect.origin = [[self window] convertBaseToScreen:rect.origin];
    #else
        if (_isNewerThanLion) rect = [[self window] convertRectToScreen:rect];
        else rect.origin = [[self window] convertBaseToScreen:rect.origin];
    #endif

    return rect;
}

- (NSPoint) centerOnScreen
{
    NSRect viewRect = [self frameOnScreen];
    return (NSPoint) {
        .x = viewRect.origin.x + floor(viewRect.size.width  / 2),
        .y = viewRect.origin.y + floor(viewRect.size.height / 2)
    };
}

- (void) updateMouse {
    if (_isMouseCaptured) {
        #ifdef USE_GNUSTEP
            int screen = [[NSScreen mainScreen] screenNumber];
            [GSCurrentServer() setMouseLocation:[self centerOnScreen]
                                       onScreen:screen];
        #endif
    } else {
        NSPoint mouseLocation = [NSEvent mouseLocation];

        static BOOL wasInView = NO;

        BOOL isInView = NSPointInRect(mouseLocation, [self frameOnScreen]);

        if (wasInView && !isInView) mouse_hover(hud_window, false);
        if (!wasInView && isInView) mouse_hover(hud_window, true);

        wasInView = isInView;
    }
}

- (void) captureMouse {
    if (!_isMouseCaptured) {
        #ifdef USE_GNUSTEP
            int win = [[self window] windowNumber];

            [GSCurrentServer() hidecursor];
            [GSCurrentServer() capturemouse:win];
        #endif

        _isMouseCaptured = true;
    }
}

- (void) releaseMouse {
    if (_isMouseCaptured) {
        #ifdef USE_GNUSTEP
            [GSCurrentServer() showcursor];
            [GSCurrentServer() releasemouse];
        #endif

        _isMouseCaptured = false;
    }
}

- (void) sendMouseEvent:(NSEvent *) ev
{
    if (_isMouseCaptured) {
        NSPoint point = [NSEvent mouseLocation], center = [self centerOnScreen];
        double dx = point.x - center.x, dy = point.y - center.y;

        mouse(hud_window, dx, -dy);
    } else {
        NSPoint point = [self convertPoint:[ev locationInWindow] fromView:nil];
        NSRect rect = [self frame];

        mouse(hud_window, point.x, rect.size.height - point.y);
    }
}

- (BOOL) isMouseCaptured
{
    return _isMouseCaptured;
}

- (void) mouseMoved:(NSEvent *) ev
{
    [self sendMouseEvent:ev];
}

- (void) mouseDragged:(NSEvent *) ev
{
    [self sendMouseEvent:ev];
}

- (void) rightMouseDragged:(NSEvent *) ev
{
    [self sendMouseEvent:ev];
}

- (void) otherMouseDragged:(NSEvent *) ev
{
    [self sendMouseEvent:ev];
}

- (void) mouseUp:(NSEvent *) ev
{
    mouse_click(hud_window, WINDOW_MOUSE_LMB, WINDOW_RELEASE, 0);
}

- (void) mouseDown:(NSEvent *) ev
{
    mouse_click(hud_window, WINDOW_MOUSE_LMB, WINDOW_PRESS, 0);
}

- (void) rightMouseUp:(NSEvent *) ev
{
    mouse_click(hud_window, WINDOW_MOUSE_RMB, WINDOW_RELEASE, 0);
}

- (void) rightMouseDown:(NSEvent *) ev
{
    mouse_click(hud_window, WINDOW_MOUSE_RMB, WINDOW_PRESS, 0);
}

- (void) scrollWheel:(NSEvent *) ev
{
    float dx = [ev deltaX], dy = [ev deltaY];

    if (fabs(dx) > 0.0f || fabs(dy) > 0.0f)
        mouse_scroll(hud_window, dx, dy);
}

- (void) keyUp:(NSEvent *) ev
{
    window_sendkey(WINDOW_RELEASE, [ev keyCode], [ev modifierFlags] & NSControlKeyMask);
}

- (void) keyDown:(NSEvent *) ev
{
    window_sendkey(WINDOW_PRESS, [ev keyCode], [ev modifierFlags] & NSControlKeyMask);
    text_input(hud_window, (const uint8_t *) [[ev characters] UTF8String]);
}

- (void) flagsChanged:(NSEvent *) ev
{
    static unsigned int previousFlags = 0;

    unsigned int flags = [ev modifierFlags];

    int mods = flags & NSControlKeyMask;

    if ((flags & NSControlKeyMask) && !(previousFlags & NSControlKeyMask))
        window_sendkey(WINDOW_PRESS, COCOA_KEY_CONTROL, mods);

    if (!(flags & NSControlKeyMask) && (previousFlags & NSControlKeyMask))
        window_sendkey(WINDOW_RELEASE, COCOA_KEY_CONTROL, mods);

    if ((flags & NSShiftKeyMask) && !(previousFlags & NSShiftKeyMask))
        window_sendkey(WINDOW_PRESS, COCOA_KEY_SHIFT, mods);

    if (!(flags & NSShiftKeyMask) && (previousFlags & NSShiftKeyMask))
        window_sendkey(WINDOW_RELEASE, COCOA_KEY_SHIFT, mods);

    if ((flags & NSAlternateKeyMask) && !(previousFlags & NSAlternateKeyMask))
        window_sendkey(WINDOW_PRESS, COCOA_KEY_ALT, mods);

    if (!(flags & NSAlternateKeyMask) && (previousFlags & NSAlternateKeyMask))
        window_sendkey(WINDOW_RELEASE, COCOA_KEY_ALT, mods);

    previousFlags = flags;
}
@end

static BOOL isRunning = YES;

@interface GameAppDelegate : NSObject
@end

@implementation GameAppDelegate
- (NSApplicationTerminateReply) applicationShouldTerminate:(NSNotification *) notification
{
    isRunning = NO;

    return NSTerminateNow;
}

- (void) applicationWillFinishLaunching:(NSNotification *) notification
{
    NSMenu * menubar = [[NSMenu alloc] init];

    [menubar setTitle:@"BetterSpades"];
    [NSApp setMainMenu:menubar];

    NSMenuItem * quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit"
                                                           action:@selector(terminate:)
                                                    keyEquivalent:@"q"];
    [menubar addItem:quitMenuItem];

    [NSApp setApplicationIconImage:[[NSImage alloc] initWithContentsOfFile:@"icon.tiff"]];
}
@end

@interface GameWindowDelegate : NSObject
@end

@implementation GameWindowDelegate
- (void) windowWillClose:(NSNotification *) notification
{
    isRunning = NO;
}

- (void) windowDidBecomeKey:(NSNotification *) notification
{
    mouse_focus(hud_window, true);
}

- (void) windowDidResignKey:(NSNotification *) notification
{
    mouse_focus(hud_window, false);
}

- (void) windowDidResize:(NSNotification *) notification
{
    NSWindow * window = [notification object];

    NSRect rect = [[window contentView] frame];
    reshape(hud_window, rect.size.width, rect.size.height);
}
@end

static NSApplication * app;
static NSWindow      * window;
static GameView      * view;

void window_mousemode(int mode) {
    if (mode == WINDOW_CURSOR_ENABLED)
        [view releaseMouse];

    if (mode == WINDOW_CURSOR_DISABLED)
        [view captureMouse];
}

int window_get_mousemode() {
    return [view isMouseCaptured] ? WINDOW_CURSOR_DISABLED : WINDOW_CURSOR_ENABLED;
}

void window_settitle(char * title) {
    [window setTitle:[NSString stringWithUTF8String:title]];
}

void window_mouseloc(double * x, double * y) {
    NSPoint locationInWindow = [window mouseLocationOutsideOfEventStream];
    NSPoint locationInView = [view convertPoint:locationInWindow fromView:nil];

    NSRect viewRect = [view frame];

    *x = locationInView.x;
    *y = viewRect.size.height - locationInView.y;
}

void window_fromsettings() {
    NSSize newSize = {.width = settings.window_width, .height = settings.window_height};
    [window setContentSize:newSize];

    NSRect viewRect = [view frame];
    reshape(hud_window, viewRect.size.width, viewRect.size.height);
}

float window_time() {
    static NSTimeInterval offset = -1;
    if (offset < 0) offset = [NSDate timeIntervalSinceReferenceDate];

    return (float) ([NSDate timeIntervalSinceReferenceDate] - offset);
}

const char * window_clipboard() {
    return NULL; // TODO
}

void window_textinput(int allow) {
}

void window_swapping(int value) {
}

void window_deinit() {
}

void window_keyname(int keycode, char * output, size_t length) {
    snprintf(output, length, "#%x", keycode); // TODO
}

void window_init(int * argc, char ** argv) {
    app = [NSApplication sharedApplication];

    unsigned int windowStyle = NSTitledWindowMask | NSClosableWindowMask | NSResizableWindowMask;
    NSRect windowRect = NSMakeRect(0, 0, settings.window_width, settings.window_height);

    [NSAutoreleasePool new];

    window = [[NSWindow alloc] initWithContentRect:windowRect
                                         styleMask:windowStyle
                                           backing:NSBackingStoreBuffered
                                             defer:NO];
    [window autorelease];

    [window setTitle:@"TigerSpades"];

    [app setDelegate:[GameAppDelegate alloc]];

    [window setDelegate:[GameWindowDelegate alloc]];

    NSOpenGLPixelFormatAttribute viewAttrs[] = {NSOpenGLPFADoubleBuffer, 0};
    NSOpenGLPixelFormat * format = [[NSOpenGLPixelFormat alloc] initWithAttributes:viewAttrs];

    view = [[GameView alloc] initWithFrame:windowRect pixelFormat:format];
    [view autorelease];

    [format release];

    [window setContentView:view];

    [window orderFrontRegardless];

    [window setAcceptsMouseMovedEvents:YES];

    [[view openGLContext] makeCurrentContext];

    [app finishLaunching];
}

void window_eventloop(Idle idle, Render render) {
    double t = window_time();

    while (isRunning) {
        double dt = window_time() - t; t += dt; fps = 1.0F / dt;

        idle(dt);
        render();

        NSAutoreleasePool * pool = [NSAutoreleasePool new];

        for (;;) {
            NSEvent * ev = [app nextEventMatchingMask:NSAnyEventMask
                                            untilDate:[NSDate distantPast]
                                               inMode:NSDefaultRunLoopMode
                                              dequeue:YES];

            if (ev == nil) break;

            [app sendEvent:ev];
        }

        [pool release];

        [view updateMouse];

        [[view openGLContext] flushBuffer];
    }
}

#else

typedef void dummy;

#endif
