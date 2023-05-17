#!/usr/bin/env bash
set -Eeuo pipefail
trap trap_sigint SIGINT
trap cleanup SIGTERM ERR EXIT
cd "$(dirname "${BASH_SOURCE[0]}")/../" &>/dev/null

# shellcheck disable=SC2120
help () {
    # if arguments, print them
    [ $# == 0 ] || echo "$*"

  cat <<EOF
Usage: $(basename "${BASH_SOURCE[0]}") [OPTION...]
Available options:
  -h, --help                 display this help and exit
      --jlinkexe=EXE         path to JLinkExe. Defaults to assuming it's in the path
      --jlinkrttclient=EXE   path to JLinkRTTClient. Defaults to assuming it's in the path
      --device=ID            serial ID of JLink to talk to
      --server-only          only run JLink server
EOF

    # if args, exit 1 else exit 0
    [ $# == 0 ] || exit 1
    exit 0
}

#shellcheck disable=SC2034
setup_colors() {
    if [[ -t 2 ]] && [[ -z "${NO_COLOR-}" ]] && [[ "${TERM-}" != "dumb" ]]; then
        NOFORMAT="$(tput sgr0)"
        RED="$(tput setaf 1)"
        GREEN="$(tput setaf 2)"
        ORANGE="$(tput setaf 3)"
        BLUE="$(tput setaf 4)"
        PURPLE="$(tput setaf 5)"
        CYAN="$(tput setaf 6)"
        YELLOW="$(tput setaf 7)"
    else
        NOFORMAT='' RED='' GREEN='' ORANGE='' BLUE='' PURPLE='' CYAN='' YELLOW=''
    fi
}

setup_colors
msg() {
    echo >&2 -e "${1-}"
}

die() {
    local msg=$1
    local code=${2-1} # default exit status 1
    msg "$msg"
    exit "$code"
}

function cleanup() {
    trap - SIGINT SIGTERM ERR EXIT
    ids=$(jobs -rp)
    if [[ -n "$ids" ]]; then
        # shellcheck disable=SC2086
        kill $ids
    fi
    exit "${1:-42}"
}
function trap_sigint() {
    echo "SIGINT"
    cleanup 130
}



# getopt short options go together, long options have commas
TEMP=$(getopt -o h --long help,server-only,jlinkexe:,jlinkrttclient:,device: -n "$0" -- "$@")
#shellcheck disable=SC2181
if [ $? != 0 ] ; then
    die "something wrong with getopt"
fi
eval set -- "$TEMP"

JLinkExe=JLinkExe
JLinkRTTClient=JLinkRTTClient
device=
serverOnly=false
while true ; do
    case "$1" in
        -h|--help) help; exit 0; shift ;;
        --server-only) serverOnly=true ; shift ;;
        --jlinkexe) JLinkExe=$2 ; shift 2 ;;
        --jlinkrttclient) JLinkRTTClient=$2 ; shift 2 ;;
        --device) device=$2 ; shift 2 ;;
        --) shift ; break ;;
        *) die "issue parsing args, unexpected argument '$0'!" ;;
    esac
done


if pgrep JLinkExe ; then
    msg "${ORANGE}WARNING: other JLinkExe processes are running, you might care?${NOFORMAT}"
    sleep 1;
fi

function versionGreaterThan() {
    a=$1
    b=$2
    [[ "$(echo -e "$b\n$a" | sort -V | head -n1)" == "$b" ]]
}

# does JLinkExe support -nogui?
jlinkexeFlags=
version=$( (timeout 0.1 JLinkExe --version 2>&1 || true) | sed -n 's/SEGGER J-Link Commander V\([^ ]*\) .*/\1/p')
if versionGreaterThan "$version" "6.80"; then
    jlinkexeFlags="$jlinkexeFlags -nogui 1"
fi
if ! versionGreaterThan "$version" "6.56"; then
    die "need JLinkExe greater than 6.56 for nRF5340 support"
fi

# https://unix.stackexchange.com/a/358101/60480
# cross-platform!
port=$( netstat -an | awk '
  $6 == "LISTEN" {
    if ($4 ~ "[.:][0-9]+$") {
      split($4, a, /[:.]/);
      port = a[length(a)];
      p[port] = 1
    }
  }
  END {
    for (i = 3000; i < 65000 && p[i]; i++){};
    if (i == 65000) {exit 1};
    print i
  }
')

msg "rtt telnet port is $port"

# https://wiki.segger.com/J-Link_Commander#Batch_processing
if [[ -n "$device" ]]; then
    jlinkexeFlags="$jlinkexeFlags -USB $device"
fi

function jlinkServer() {
  # shellcheck disable=SC2086
  "$JLinkExe" -if SWD -speed 4000 -device nrf52840_xxAA -autoconnect 1 \
       -RTTTelnetPort "$port" $jlinkexeFlags
}

if [ "$serverOnly" == true ]; then
    jlinkServer
else
    jlinkServer >/dev/null &
    sleep 1 # need to wait for jLinkServer to start before continuing

    if command -v nc >/dev/null 2>&1 ; then
        nc localhost "$port"
    else
        $JLinkRTTClient -RTTTelnetPort "$port"
    fi
fi

#probably can't get here, need to ctrl c out above
exit 1;
