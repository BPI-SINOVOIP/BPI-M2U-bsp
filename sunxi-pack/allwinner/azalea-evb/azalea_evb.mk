$(call inherit-product-if-exists, target/allwinner/azalea-common/azalea-common.mk)

PRODUCT_PACKAGES +=

PRODUCT_COPY_FILES +=

PRODUCT_AAPT_CONFIG := large xlarge hdpi xhdpi
PRODUCT_AAPT_PERF_CONFIG := xhdpi
PRODUCT_CHARACTERISTICS := musicbox

PRODUCT_BRAND := allwinner
PRODUCT_NAME := azalea_evb
PRODUCT_DEVICE := azalea-evb
PRODUCT_MODEL := Azalea R40 Evb
