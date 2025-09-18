#!/bin/sh
# Seed proxy settings into the e2e test filesystem so the Alpine package
# installation can run behind an explicit HTTP proxy.

set -eu

usage() {
    cat <<'USAGE'
Usage: e2e-proxy-bootstrap.sh [--http URL] [--https URL] [--no-proxy HOSTS]

Reads proxy values from the environment (http_proxy/https_proxy/no_proxy) or
from the provided options, then runs the Alpine package installation inside the
emulator while exporting those variables. The script expects to be executed from
anywhere inside the repository after `ninja -C build` has produced `build/ish`
and after the e2e harness has created `e2e_out/testfs`.
USAGE
}

escape_sh() {
    printf "%s" "$1" | sed "s/'/'\\''/g"
}

SCRIPT_DIR=$(CDPATH="" cd -- "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(CDPATH="" cd -- "$SCRIPT_DIR/.." && pwd)
ISH_BIN="$PROJECT_ROOT/build/ish"
TESTFS_DIR="$PROJECT_ROOT/e2e_out/testfs"
PACKAGES="build-base python2 python3"

HTTP_PROXY_VALUE=${http_proxy-${HTTP_PROXY-}}
HTTPS_PROXY_VALUE=${https_proxy-${HTTPS_PROXY-}}
NO_PROXY_VALUE=${no_proxy-${NO_PROXY-}}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --http)
            shift || { echo "Missing value for --http" >&2; usage; exit 1; }
            HTTP_PROXY_VALUE=$1
            ;;
        --http=*)
            HTTP_PROXY_VALUE=${1#--http=}
            ;;
        --https)
            shift || { echo "Missing value for --https" >&2; usage; exit 1; }
            HTTPS_PROXY_VALUE=$1
            ;;
        --https=*)
            HTTPS_PROXY_VALUE=${1#--https=}
            ;;
        --no-proxy)
            shift || { echo "Missing value for --no-proxy" >&2; usage; exit 1; }
            NO_PROXY_VALUE=$1
            ;;
        --no-proxy=*)
            NO_PROXY_VALUE=${1#--no-proxy=}
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unrecognized option: $1" >&2
            usage
            exit 1
            ;;
    esac
    shift
    done

if [ -z "$HTTP_PROXY_VALUE" ] && [ -z "$HTTPS_PROXY_VALUE" ]; then
    echo "error: provide a proxy with --http/--https or set http_proxy/https_proxy in the environment." >&2
    exit 1
fi

if [ -n "$HTTP_PROXY_VALUE" ] && [ -z "$HTTPS_PROXY_VALUE" ]; then
    HTTPS_PROXY_VALUE=$HTTP_PROXY_VALUE
fi

if [ ! -x "$ISH_BIN" ]; then
    echo "error: $ISH_BIN not found or not executable. Run 'ninja -C build' first." >&2
    exit 1
fi

if [ ! -d "$TESTFS_DIR" ]; then
    echo "error: $TESTFS_DIR is missing. Run 'meson test -C build' once to bootstrap the filesystem." >&2
    exit 1
fi

inner_script="set -e;"

if [ -n "$HTTP_PROXY_VALUE" ]; then
    esc=$(escape_sh "$HTTP_PROXY_VALUE")
    inner_script="$inner_script export http_proxy='$esc'; export HTTP_PROXY='$esc';"
fi

if [ -n "$HTTPS_PROXY_VALUE" ]; then
    esc=$(escape_sh "$HTTPS_PROXY_VALUE")
    inner_script="$inner_script export https_proxy='$esc'; export HTTPS_PROXY='$esc';"
fi

if [ -n "$NO_PROXY_VALUE" ]; then
    esc=$(escape_sh "$NO_PROXY_VALUE")
    inner_script="$inner_script export no_proxy='$esc'; export NO_PROXY='$esc';"
fi

inner_script="$inner_script apk update && apk add $PACKAGES"

echo "Seeding proxy configuration into $TESTFS_DIR via $ISH_BIN"
printf 'Using http_proxy=%s\n' "${HTTP_PROXY_VALUE:-<unset>}"
printf 'Using https_proxy=%s\n' "${HTTPS_PROXY_VALUE:-<unset>}"
if [ -n "$NO_PROXY_VALUE" ]; then
    printf 'Using no_proxy=%s\n' "$NO_PROXY_VALUE"
fi

exec "$ISH_BIN" -f "$TESTFS_DIR" /bin/sh -lc "$inner_script"
