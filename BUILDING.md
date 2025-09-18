# Building iSH

This guide summarizes how to set up the source tree and build either the command-line emulator or the iOS application.

## 1. Repository setup

1. Clone the repository with its submodules: `git clone --recurse-submodules <repo-url>`.
2. If you already cloned without submodules, run `git submodule update --init` from the repository root to populate them.

## 2. Toolchain prerequisites

Install the following tools before building:

- Python 3 with the Meson build system: `pip3 install meson`.
- Ninja build tool.
- Clang and LLD toolchain (macOS: `brew install llvm`; Debian/Ubuntu: `sudo apt install clang lld`; Arch: `sudo pacman -S clang lld`).
- SQLite development files (e.g., `sudo apt install libsqlite3-dev`).
- libarchive development files (e.g., `brew install libarchive`, `sudo port install libarchive`, or `sudo apt install libarchive-dev`).
- `pkg-config` if your distribution does not include it by default.

To exercise the end-to-end Meson tests you will also need common userland tools on the host system, notably `wget` (used to grab the Alpine minirootfs) and `bc` (used by the test harness when summarizing results).

macOS developers will also need the latest Xcode in order to compile and sign the iOS application.

## 3. Building the command-line emulator (Linux/macOS)

1. Create a dedicated build directory: `meson setup build`.
2. Compile the project: `ninja -C build`.

The primary emulator binary is placed at `build/ish`. Supporting utilities such as `fakefsify` are generated under `build/tools/`.

### 3.1 Configuring optional features

Meson options allow you to customize the build. For example:

- Enable logging channels: `meson configure build -Dlog="strace verbose"`.
- Change the emulator engine: `meson configure build -Dengine=unicorn` (defaults to `asbestos`).
- Switch to the Linux kernel shim instead of the default iSH kernel: `meson configure build -Dkernel=linux`.
- Disable carriage-return/line-feed translation: `meson configure build -Dno_crlf=true`.

Run `meson configure build` without arguments to inspect the current configuration, including options such as `nolog`, `log_handler`, `kconfig`, and `vdso_c_args`.

## 4. Running tests

After building, execute the test suite with `ninja -C build test` (or `meson test -C build`). This runs both the floating-point self-test and the end-to-end smoke tests.

The first end-to-end test invocation bootstraps a minimal Alpine Linux filesystem under `e2e_out/testfs`. During this bootstrap the harness downloads a minirootfs tarball with `wget` and, inside the emulator, runs `apk update && apk add build-base python2 python3`. Ensure your environment permits outbound HTTP traffic so that these package downloads succeed; if the network bootstrap fails, delete `e2e_out/testfs` before re-running the suite so that the setup step is retried.

## 5. Preparing a root filesystem

To obtain a runnable root filesystem, download an Alpine Linux i386 minirootfs tarball and import it using the bundled tool:

```
./build/tools/fakefsify /path/to/alpine-minirootfs.tar.gz alpine
```

Run the emulator against that filesystem with:

```
./build/ish -f alpine /bin/sh
```

If `fakefsify` is missing, verify that libarchive development headers are installed and rebuild the project.

## 6. Building the iOS application

1. Open `iSH.xcodeproj` in Xcode.
2. Edit `iSH.xcconfig` and set `ROOT_BUNDLE_IDENTIFIER` to a bundle identifier you control.
3. In the project (not target) build settings, assign your Apple developer team ID.
4. Select the desired scheme/device and click **Run** to build and deploy. Project scripts handle the remaining build steps automatically.

If you encounter issues building the app, consult the project README or open an issue for assistance.

## 7. Further reading

The top-level `README.md` contains additional background information, links to community resources, and localized READMEs. The project wiki offers tutorials and troubleshooting tips.
