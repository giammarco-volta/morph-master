# Curve editing and MIDI regression checks

From a Visual Studio x64 development shell:

```powershell
cmake --preset x64-debug -DMORPHORA_BUILD_TESTS=ON
cmake --build out/build/x64-debug --parallel 6
$env:PATH = 'E:\Qt\6.8.3\msvc2022_64\bin;' + $env:PATH
ctest --test-dir out/build/x64-debug --output-on-failure
```

Routing tests use a recording MIDI output. They cover unchanged held Poly
voices, Mono Expression without retriggers, reserved CC rejection, first-message
anchoring, relative deltas, boundary reversal, manual edits, and queued edits
surviving older snapshots. All integer endpoint pairs are checked for parameter
round trips and slope constraints. Preset tests use isolated storage and check
fractional endpoint persistence and legacy preset loading.

The offscreen UI test loads the actual curve page, exercises inline Position
editing and simulated incoming MIDI, and renders desktop, tablet and phone views.
No physical MIDI ports are selected. Captures are in the build tests directory.

For hardware listening: assign the Pa5X slider/pedal CC in the curve editor.
Verify that its first message leaves the curve unchanged, later movement edits
it, reversal at a limit responds immediately, and manual point edits do not
cause the next hardware movement to jump. Repeat in Mono and Poly; save/reload
the preset to verify the curve and assignments are retained.
