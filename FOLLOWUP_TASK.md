# Follow-up Task: Validate Network-Dependent Tests

With outbound Internet access now confirmed, continue from the BUILDING.md guide and exercise the network-dependent test workflow.

1. Ensure prerequisites from the "Toolchain prerequisites" section of BUILDING.md are installed, including wget and bc on the host.
2. Configure a fresh build directory if needed: `meson setup build`.
3. Compile the project: `ninja -C build`.
4. Run the full test suite, allowing the e2e bootstrap to download Alpine packages: `meson test -C build`.
5. If any tests fail, capture the logs from `build/meson-logs/testlog.txt`, diagnose the issue, and update BUILDING.md with any newly discovered requirements or workarounds.
6. Report the successful run (or remaining issues) so the documentation can be finalized with verified instructions.

### New investigation: package bootstrap still fails behind proxies

The current e2e bootstrap asks the emulator to run `apk update && apk add build-base python2 python3` without exporting any proxy settings. In corporate or sandboxed environments (like this one) outbound HTTP/HTTPS traffic must flow through `proxy:8080`, so the package downloads fail with:

```
ERROR: http://dl-cdn.alpinelinux.org/alpine/v3.11/main: network error (check Internet connection and firewall)
```

Because that step never installs `build-base`, GCC is missing when later e2e scenarios compile helper binaries. Teach the harness to pass the host's `http_proxy`, `https_proxy`, and `no_proxy` values through to the `$ISH` invocation in `tests/e2e/e2e.bash` (or reuse `tools/e2e-proxy-bootstrap.sh`) so the initial `apk` invocation succeeds automatically.
