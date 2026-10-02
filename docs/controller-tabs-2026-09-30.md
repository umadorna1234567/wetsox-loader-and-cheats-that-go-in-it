# Just Cause 4 tabs and native controller hotkeys

- Replaced the JC4 Movement tab with Grapple (unlimited range and speed) and Hoverboard (speed). Existing config keys and gameplay wiring are preserved.
- Added native DirectInput reads for Sony DualSense, DualSense Edge, and official DS4 v1/v2, alongside four existing XInput slots. This fixes the connected DualSense being invisible to the XInput-only recorder.
- Normalized Sony buttons, POV, trigger axes, and stick directions to the existing Pad binding codes, shared by the recorder and all three game modules. Native controllers use nonexclusive background acquisition; polling reacquires lost devices and periodically checks connection changes.
- Known Sony USB product IDs only; other native controller layouts are not guessed. Existing XInput support remains available.
- Input capture tests exercise native slots 4 and 7, pre-held input suppression, rearming and cancellation. Mapping tests cover Cross, L2, trigger dead zone, diagonal D-pad, right stick axes and device identification.
- JC4 UI smoke verifies that Movement is absent and Grapple/Hoverboard expose their controls.

Validation: all 15 CTest tests passed (test-controller-tabs.log). The connected DualSense was acquired successfully and physical L2, R2 and Cross presses produced Pad LT, Pad RT and Pad A through the shared native reader. End-to-end activation inside the game still needs a user check after restarting with the updated DLL.
