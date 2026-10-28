# System76 EC
#   Add System76 EC raw HID command interface
VALID_SYSTEM76_EC_SETTINGS := yes no
ifeq ($(strip $(VIA_ENABLE)),yes)
    SYSTEM76_EC_ENABLE ?= no
else
    SYSTEM76_EC_ENABLE ?= yes
endif
ifneq ($(strip $(SYSTEM76_EC_ENABLE)),yes)
    ifeq ($(filter $(SYSTEM76_EC_ENABLE),$(VALID_SYSTEM76_EC_SETTINGS)),)
        $(error SYSTEM76_EC_ENABLE="$(strip $(SYSTEM76_EC_ENABLE))" is not a valid setting for the System76 EC option)
    endif
    ifeq ($(strip $(SYSTEM76_EC_ENABLE)),no)
        ifneq ($(strip $(VIA_ENABLE)),yes)
            RAW_ENABLE := no
        endif
    endif
else
    ifneq ($(strip $(VIA_ENABLE)),yes)
        OPT_DEFS += -DSYSTEM76_EC
    else
        $(warning SYSTEM76_EC_ENABLE="$(strip $(SYSTEM76_EC_ENABLE))" and VIA_ENABLE="$(strip $(VIA_ENABLE))" are mutually exclusive, disabling SYSTEM76_EC_ENABLE)
        SYSTEM76_EC_ENABLE := no
    endif
endif

# RGB matrix per-layer control
#   Add RGB matrix per-layer control into VIA
VALID_RGB_PER_LAYER_SETTINGS := yes no
RGB_PER_LAYER_ENABLE ?= no
ifeq ($(filter $(strip $(RGB_PER_LAYER_ENABLE)),$(VALID_RGB_PER_LAYER_SETTINGS)),)
    $(error RGB_PER_LAYER_ENABLE="$(strip $(RGB_PER_LAYER_ENABLE))" is not a valid setting for the RGB matrix per-layer option)
endif
ifeq ($(strip $(RGB_PER_LAYER_ENABLE)),yes)
    OPT_DEFS += -DRGB_PER_LAYER
endif

# USB multiplexer
#   Allow variable-length arrays in USB multiplexer's I2C functions
VALID_USB_VLA_SETTINGS := yes no
USB_VLA_ENABLE ?= no
ifeq ($(filter $(strip $(USB_VLA_ENABLE)),$(VALID_USB_VLA_SETTINGS)),)
    $(error USB_VLA_ENABLE="$(strip $(USB_VLA_ENABLE))" is not a valid setting for the USB multiplexer VLA option)
endif
ifeq ($(strip $(USB_VLA_ENABLE)),yes)
    OPT_DEFS += -DUSB_VLA
endif
