#!/bin/bash
#
# SPDX-FileCopyrightText: 2016 The CyanogenMod Project
# SPDX-FileCopyrightText: 2017-2024 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

set -e

DEVICE=spes
VENDOR=xiaomi

# Load extract_utils and do some sanity checks
MY_DIR="${BASH_SOURCE%/*}"
if [[ ! -d "${MY_DIR}" ]]; then MY_DIR="${PWD}"; fi

ANDROID_ROOT="${MY_DIR}/../../.."

# If XML files don't have comments before the XML header, use this flag
# Can still be used with broken XML files by using blob_fixup
export TARGET_DISABLE_XML_FIXING=true

HELPER="${ANDROID_ROOT}/tools/extract-utils/extract_utils.sh"
if [ ! -f "${HELPER}" ]; then
    echo "Unable to find helper script at ${HELPER}"
    exit 1
fi
source "${HELPER}"

# Default to sanitizing the vendor folder before extraction
CLEAN_VENDOR=true

KANG=
SECTION=

while [ "${#}" -gt 0 ]; do
    case "${1}" in
        --only-common)
            ONLY_COMMON=true
            ;;
        --only-firmware)
            ONLY_FIRMWARE=true
            ;;
        --only-target)
            ONLY_TARGET=true
            ;;
        -n | --no-cleanup)
            CLEAN_VENDOR=false
            ;;
        -k | --kang)
            KANG="--kang"
            ;;
        -s | --section)
            SECTION="${2}"
            shift
            CLEAN_VENDOR=false
            ;;
        *)
            SRC="${1}"
            ;;
    esac
    shift
done

if [ -z "${SRC}" ]; then
    SRC="adb"
fi

function blob_fixup() {
    case "${1}" in
        vendor/lib64/vendor.qti.hardware.camera.postproc@1.0-service-impl.so)
            [ "$2" = "" ] && return 0
            "${SIGSCAN}" -p "13 0A 00 94" -P "1F 20 03 D5" -f "${2}"
            ;;
        vendor/lib64/camera/components/com.qti.node.mialgocontrol.so)
            [ "$2" = "" ] && return 0
            llvm-strip --strip-debug  "${2}"
			"${PATCHELF}" --add-needed "libpiex_shim.so" "${2}"
            ;;
        vendor/lib64/hw/camera.qcom.so)
            [ "$2" = "" ] && return 0
            llvm-strip --strip-debug  "${2}"
			"${PATCHELF}" --add-needed "libcamxmmaphook.so" "${2}"
            ;;
        vendor/bin/hw/vendor.qti.hardware.display.composer-service)
            [ "$2" = "" ] && return 0
            "${SIGSCAN}" -p "FF 03 01 D1 FD 7B 01 A9 FD 43 00 91 F5 13 00 F9" -P "C0 03 5F D6 FD 7B 01 A9 FD 43 00 91 F5 13 00 F9" -f "${2}"
            ;;
        odm/lib64/libmt@1.3.so | odm/lib64/vendor.xiaomi.hardware.mtdservice@1.3.so)
            [ "$2" = "" ] && return 0
            python3 -c "import os; p = '${2}'; data = open(p, 'rb').read(); data = data.replace(b'_ZN15keymasterdevice21KeymasterKeyProvision28SetProvisioningKeyboxSuccessEv', b'_ZN15keymasterdevice21KeymasterKeyProvision24SetProvisioniningSuccessEv\x00\x00\x00\x00'); open(p, 'wb').write(data)"
            ;;
        *)
            return 1
            ;;
    esac

    return 0
}

function blob_fixup_dry() {
    blob_fixup "$1" ""
}

# Initialize the helper
setup_vendor "${DEVICE}" "${VENDOR}" "${ANDROID_ROOT}" false "${CLEAN_VENDOR}"

extract "${MY_DIR}/proprietary-files.txt" "${SRC}" "${KANG}" --section "${SECTION}"

"${MY_DIR}/setup-makefiles.sh"
