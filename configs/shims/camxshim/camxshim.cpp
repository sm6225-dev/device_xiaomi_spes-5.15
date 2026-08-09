#include <stdint.h>
#include <android/log.h>

#define LOG_TAG "CamX-ExtFormatShim"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define CAMXFORMAT_VISIBILITY_PUBLIC __attribute__ ((visibility ("default")))

typedef enum {
    CamxFormatResultSuccess = 0,
    CamxFormatResultEFailed = 1,
} CamxFormatResult;

typedef enum {
    CAMERA_PLANE_TYPE_RAW,
    CAMERA_PLANE_TYPE_Y,
    CAMERA_PLANE_TYPE_UV,
    CAMERA_PLANE_TYPE_U,
    CAMERA_PLANE_TYPE_V,
    CAMERA_PLANE_TYPE_META_Y,
    CAMERA_PLANE_TYPE_META_VU,
} CamxPlaneType;

typedef enum {
    CAMERA_PIXEL_FORMAT_NV21_ZSL = 0x113,
    CAMERA_PIXEL_FORMAT_YUV_FLEX = 0x125,
    CAMERA_PIXEL_FORMAT_UBWC_FLEX = 0x126,
    CAMERA_PIXEL_FORMAT_MULTIPLANAR_FLEX = 0x127,
    CAMERA_PIXEL_FORMAT_NV12_VENUS = 0x7FA30C04,
    CAMERA_PIXEL_FORMAT_YCbCr_420_SP_UBWC = 0x7FA30C06,
    CAMERA_PIXEL_FORMAT_YCbCr_420_TP10_UBWC = 0x7FA30C09,
} CamxPixelFormat;

#define CamxFormatUtilMaxFlexibleFormats 10

typedef struct {
    int             size;
    unsigned int    count;
    CamxPixelFormat pixelFormat[CamxFormatUtilMaxFlexibleFormats];
    int             strideAlignment[CamxFormatUtilMaxFlexibleFormats];
    int             scanlineAlignment[CamxFormatUtilMaxFlexibleFormats];
} CamxFlexFormatInfo;

typedef struct {
    int          planeSize;
    int          stride;
    int          scanline;
    unsigned int alignment;
} CamxPlaneLayoutInfo;

extern "C" CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetFlexibleYUVFormats(CamxFlexFormatInfo* pFlexFormatInfo) {
    if (!pFlexFormatInfo) return CamxFormatResultEFailed;

    pFlexFormatInfo->count = 3;

    pFlexFormatInfo->pixelFormat[0] = CAMERA_PIXEL_FORMAT_YUV_FLEX;
    pFlexFormatInfo->strideAlignment[0] = 256;
    pFlexFormatInfo->scanlineAlignment[0] = 256;

    pFlexFormatInfo->pixelFormat[1] = CAMERA_PIXEL_FORMAT_NV12_VENUS;
    pFlexFormatInfo->strideAlignment[1] = 256;
    pFlexFormatInfo->scanlineAlignment[1] = 256;

    pFlexFormatInfo->pixelFormat[2] = CAMERA_PIXEL_FORMAT_UBWC_FLEX;
    pFlexFormatInfo->strideAlignment[2] = 256;
    pFlexFormatInfo->scanlineAlignment[2] = 256;

    ALOGI("Hijacked GetFlexibleYUVFormats\n");
    return CamxFormatResultSuccess;
}

#define ALIGN(x, align) (((x) + (align) - 1) & ~((align) - 1))

extern "C" CAMXFORMAT_VISIBILITY_PUBLIC
CamxFormatResult CamxFormatUtil_GetPlaneLayoutInfo(
    CamxPixelFormat       pixelFormat,
    CamxPlaneType         planeType,
    int                   width,
    int                   height,
    CamxPlaneLayoutInfo*  pPlaneInfo) {

    if (!pPlaneInfo) return CamxFormatResultEFailed;

    // By returning CamxFormatResultEFailed, we force CamX to fallback to CHINodeUtils::GetAlignment.
    // BUT we want to PREVENT it from falling back! So we must handle UBWC NV12 manually.
    // UBWC NV12 (Y)
    int y_stride = ALIGN(width, 128); // Standard linear is 128
    int y_scanline = ALIGN(height, 32);

    if (pixelFormat == CAMERA_PIXEL_FORMAT_NV21_ZSL) {
        y_stride = ALIGN(width, 64);
        y_scanline = height;
    }

    // Adjust for UBWC formats commonly used in Android 13 Gralloc (UBWC V2/V3)
    if (pixelFormat == CAMERA_PIXEL_FORMAT_YCbCr_420_SP_UBWC ||
        pixelFormat == CAMERA_PIXEL_FORMAT_UBWC_FLEX) {
        y_stride = ALIGN(width, 128);  // UBWC NV12 Y stride alignment is 128 (often)
        y_scanline = ALIGN(height, 32); // UBWC NV12 Y scanline alignment is 32 (often)
        }

        if (planeType == CAMERA_PLANE_TYPE_Y) {
            pPlaneInfo->stride = y_stride;
            pPlaneInfo->scanline = y_scanline;
            pPlaneInfo->planeSize = y_stride * y_scanline;
            pPlaneInfo->alignment = 4096;
        } else if (planeType == CAMERA_PLANE_TYPE_UV) {
            pPlaneInfo->stride = y_stride;
            pPlaneInfo->scanline = ALIGN(height / 2, 16);
            if (pixelFormat == CAMERA_PIXEL_FORMAT_NV21_ZSL) {
                pPlaneInfo->scanline = y_scanline / 2;
            }
            if (pixelFormat == CAMERA_PIXEL_FORMAT_YCbCr_420_SP_UBWC ||
                pixelFormat == CAMERA_PIXEL_FORMAT_UBWC_FLEX) {
                pPlaneInfo->scanline = ALIGN(height / 2, 16); // UBWC UV scanline alignment is 16
                }
                pPlaneInfo->planeSize = pPlaneInfo->stride * pPlaneInfo->scanline;
            pPlaneInfo->alignment = 4096;
        } else if (planeType == CAMERA_PLANE_TYPE_META_Y) {
            pPlaneInfo->stride = ALIGN(width, 64);
            pPlaneInfo->scanline = ALIGN(height, 16);
            pPlaneInfo->planeSize = ALIGN(pPlaneInfo->stride * pPlaneInfo->scanline, 4096);
            pPlaneInfo->alignment = 4096;
        } else if (planeType == CAMERA_PLANE_TYPE_META_VU) {
            pPlaneInfo->stride = ALIGN(width, 64);
            pPlaneInfo->scanline = ALIGN(height / 2, 16);
            pPlaneInfo->planeSize = ALIGN(pPlaneInfo->stride * pPlaneInfo->scanline, 4096);
            pPlaneInfo->alignment = 4096;
        } else {
            return CamxFormatResultEFailed;
        }

        ALOGI("GetPlaneLayoutInfo (fmt=0x%x, type=%d): stride=%d, scanline=%d, size=%d\n",
              pixelFormat, planeType, pPlaneInfo->stride, pPlaneInfo->scanline, pPlaneInfo->planeSize);

        return CamxFormatResultSuccess;
    }
