# Follow-up Task: Validate Network-Dependent Tests

With outbound Internet access now confirmed, continue from the BUILDING.md guide and exercise the network-dependent test workflow.

1. Ensure prerequisites from the "Toolchain prerequisites" section of BUILDING.md are installed, including wget and bc on the host.
2. Configure a fresh build directory if needed: `meson setup build`.
3. Compile the project: `ninja -C build`.
4. Run the full test suite, allowing the e2e bootstrap to download Alpine packages: `meson test -C build`.
5. If any tests fail, capture the logs from `build/meson-logs/testlog.txt`, diagnose the issue, and update BUILDING.md with any newly discovered requirements or workarounds.
6. Report the successful run (or remaining issues) so the documentation can be finalized with verified instructions.

This follow-up assumes stable Internet access so that the emulator can fetch Alpine packages during the e2e setup phase.
