# Far Cry compatibility and aim update ? 2026-10-01

User explicitly requested attempting unverified Far Cry engine builds after a tester reported a successful hash bypass. Both loader preflight and DLL startup now permit mismatching hashes. Missing engine/read failures and runtime hook/read/camera checks remain. This does not establish universal compatibility; FC4/FC5 still use build-specific bindings. JC4 checks are unchanged.

FC5 alignment previously projected a point 50 metres ahead and rejected normalized screen error over 0.05. Zoom and aspect scaling changed that result for the same world-space misalignment. It now extracts perspective camera orientation, checks eye proximity within 3 metres, and uses a 3-degree angle limit. The existing pose-change checks and correction limits remain. FC4 shares the extracted math with unchanged thresholds and compensation behavior.

The installed backend/Collect-game-diagnostics.ps1 generates engine and section fingerprints and samples the already loaded module status for support. It never starts a cheat or modifies settings. Reports are written outside the backend folder and excluded from upload packages. Live failure capture and alternate-engine gameplay validation require affected users; no remote issue is claimed fixed by the local tests alone.
