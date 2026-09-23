## 2026-04-12

- Switched the drag-start backing conversion to `NSView.convertToBacking(...)` so the x86_64 macOS 10.13 cross-target no longer depends on the newer `NSWindow.convertPointToBacking(...)` API.
- Added macOS 10.14 availability handling for newer title-bar materials, falling back to `windowBackground` on the x86_64 macOS 10.13 cross-target.
