#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: The LineageOS Project
# SPDX-License-Identifier: Apache-2.0
#

from extract_utils.fixups_blob import (
    blob_fixup,
    blob_fixups_user_type,
)
from extract_utils.fixups_lib import (
    lib_fixups,
    lib_fixups_user_type,
)
from extract_utils.main import (
    ExtractUtils,
    ExtractUtilsModule,
)

namespace_imports = [
    'device/xiaomi/spes',
    'hardware/qcom-caf/sm6225',
    'hardware/qcom-caf/wlan',
    'hardware/xiaomi',
    'hardware/lineage/interfaces/power-libperfmgr',
    'hardware/qcom-caf/common/libqti-perfd-client',
    'hardware/google/interfaces',
    'hardware/google/pixel',
    'vendor/qcom/opensource/commonsys/display',
    'vendor/qcom/opensource/commonsys-intf/display',
    'vendor/qcom/opensource/dataservices',
]

def lib_fixup_vendor_suffix(lib: str, partition: str, *args, **kwargs):
    return f'{lib}_vendor' if partition == 'vendor' else None

lib_fixups: lib_fixups_user_type = {
    **lib_fixups,
    (
        'com.qualcomm.qti.dpm.api@1.0',
        'vendor.qti.diaghal@1.0',
        'vendor.qti.hardware.dpmservice@1.0',
        'vendor.qti.hardware.fm@1.0',
        'vendor.qti.hardware.qccsyshal@1.0',
        'vendor.qti.hardware.qccsyshal@1.1',
        'vendor.qti.hardware.qccsyshal@1.2',
        'vendor.qti.hardware.qccvndhal@1.0',
        'vendor.qti.ImsRtpService-V1-ndk',
        'vendor.qti.imsrtpservice@3.0',
        'vendor.qti.imsrtpservice@3.1',
        'vendor.qti.qccvndhal_aidl-V1-ndk',
    ): lib_fixup_vendor_suffix,
}

blob_fixups: blob_fixups_user_type = {
    'vendor/lib64/vendor.qti.hardware.camera.postproc@1.0-service-impl.so': blob_fixup()
        .sig_replace('13 0A 00 94', '1F 20 03 D5'),
    'vendor/lib64/camera/components/com.qti.node.mialgocontrol.so': blob_fixup()
        .strip_debug_sections()
        .add_needed('libpiex_shim.so'),
    'vendor/lib64/hw/camera.qcom.so': blob_fixup()
        .strip_debug_sections()
        .add_needed('libcamxmmaphook.so'),
    'vendor/lib64/libdpps.so': blob_fixup()
        .replace_needed('libtinyxml2.so', 'libtinyxml2_1.so'),
    'vendor/lib64/vendor.libdpmframework.so': blob_fixup()
        .add_needed('libhidlbase_shim.so'),
    'vendor/lib64/libgoodixhwfingerprint.so': blob_fixup()
        .replace_needed('libvendor.goodix.hardware.biometrics.fingerprint@2.1.so', 'vendor.goodix.hardware.biometrics.fingerprint@2.1.so'),
    'vendor/lib64/libgf_ca.so': blob_fixup()
        .binary_regex_replace(
            b'/vendor/firmware_mnt/image',
            b'/vendor/firmware\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00',
        ),
    'vendor/lib64/hw/fingerprint.goodix.default.so': blob_fixup()
        .patchelf_version('0_17_2')
        .fix_soname(),
    (
    'vendor/bin/STFlashTool',
    'vendor/lib64/libstfactory-vendor.so',
    ): blob_fixup()
        .add_needed('libbase_shim.so'),
    ('vendor/etc/libnfc-nci.conf' ,'vendor/etc/libnfc-hal-st.conf'): blob_fixup()
       .regex_replace('/data/nfc', '/data/vendor/nfc'),
    'vendor/etc/seccomp_policy/c2audio.vendor.ext-arm64.policy': blob_fixup()
        .add_line_if_missing('setsockopt: 1'),
    (
    'vendor/etc/seccomp_policy/atfwd@2.0.policy',
    'vendor/etc/seccomp_policy/qms.policy',
    ): blob_fixup()
        .add_line_if_missing('gettid: 1'),
    'vendor/etc/seccomp_policy/qspm.policy': blob_fixup()
        .regex_replace('mprotect: arg2 in ~PROT_EXEC \\|\\| arg2 in ~PROT_WRITE\n', '')
        .regex_replace('sigreturn: 1\n', '')
        .regex_replace('geteuid32: 1\n', '')
        .regex_replace('getgid32: 1\n', '')
        .regex_replace('getegid32: 1\n', '')
        .regex_replace('getgroups32: 1\n', '')
        .add_line_if_missing('connect: 1')
        .add_line_if_missing('sendto: 1'),
    'vendor/lib64/libacdbloader.so': blob_fixup()
        .replace_needed('libion.so', 'libacd.so'),

}  # fmt: skip

module = ExtractUtilsModule(
    'spes',
    'xiaomi',
    blob_fixups=blob_fixups,
    lib_fixups=lib_fixups,
    namespace_imports=namespace_imports,
    check_elf=True,
)

if __name__ == '__main__':
    utils = ExtractUtils.device(module)
    utils.run()
