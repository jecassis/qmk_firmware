/*
 * Copyright (C) 2021-2022 System76
 * Copyright (C) 2021-2026 Jimmy Cassis <KernelOops@outlook.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "quantum.h"
#include "eeprom.h"

#include "system76_ec.h"
#include "usb_mux.h"

#ifdef SYSTEM76_EC
bool eeprom_is_valid(void) {
    return (eeprom_read_word(((void *)EEPROM_MAGIC_ADDR)) == EEPROM_MAGIC && eeprom_read_byte(((void *)EEPROM_VERSION_ADDR)) == EEPROM_VERSION);
}

void eeprom_set_valid(bool valid) {
    eeprom_update_word(((void *)EEPROM_MAGIC_ADDR), valid ? EEPROM_MAGIC : 0xFFFF);
    eeprom_update_byte(((void *)EEPROM_VERSION_ADDR), valid ? EEPROM_VERSION : 0xFF);
}

void bootmagic_reset_eeprom(void) {
    eeprom_set_valid(false); // Set the keyboard-specific EEPROM state as invalid
    eeconfig_disable();      // Set the TMK/QMK EEPROM state as invalid
}
#endif // SYSTEM76_EC

extern rgb_config_t layer_rgb[DYNAMIC_KEYMAP_LAYER_COUNT];

void matrix_init_kb(void) {
    usb_mux_init();

#ifdef SYSTEM76_EC
    if (!eeprom_is_valid()) {
        dynamic_keymap_reset();       // Reset the keymaps in EEPROM to what is in flash
        dynamic_keymap_macro_reset(); // Reset the macros in EEPROM to nothing
        system76_ec_rgb_eeprom(true); // Populate System76 per-layer RGB matrix settings
        eeprom_set_valid(true);       // Save the magic number last, in case saving was interrupted
    } else {
        system76_ec_rgb_eeprom(false); // Read System76 per-layer RGB matrix settings
    }
#endif // SYSTEM76_EC

    matrix_init_user();
}

#if defined(VIA_ENABLE) && defined(RGB_MATRIX_ENABLE)
void via_init_kb(void) {
    if (eeconfig_is_enabled()) {
        via_rgb_eeprom(false); // Read per-layer RGB matrix settings
    } else {
        via_rgb_eeprom(true); // Populate per-layer RGB matrix settings
    }
}
#endif // defined(VIA_ENABLE) && defined(RGB_MATRIX_ENABLE)

void housekeeping_task_kb(void) {
    usb_mux_event(); // Poll the USB hub for cable connections and disconnections
}

#define LEVEL(value) (uint8_t)((((uint16_t)(value)) * (uint16_t)RGB_MATRIX_MAXIMUM_BRIGHTNESS) / (uint16_t)255)

// clang-format off
static const uint8_t levels[] = {
    LEVEL(48),
    LEVEL(72),
    LEVEL(96),
    LEVEL(144),
    LEVEL(192),
    LEVEL(255),
};
// clang-format on

#ifdef SYSTEM76_EC
extern bool input_disabled;
static bool lctl_pressed = false, rctl_pressed = false;
#endif // SYSTEM76_EC
static uint8_t toggle_level = RGB_MATRIX_MAXIMUM_BRIGHTNESS;

static void set_value_all_layers(uint8_t value) {
    if (!system76_ec_is_unlocked()) {
        for (int8_t layer = 0; layer < DYNAMIC_KEYMAP_LAYER_COUNT; ++layer) {
            layer_rgb[layer].hsv.v = value;
        }
        system76_ec_rgb_layer(layer_state);
    }
}

static void set_mode_default_layer(uint8_t value) {
    if (!system76_ec_is_unlocked()) {
        layer_rgb[0].mode = value;
    }
    system76_ec_rgb_layer(layer_state);
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
#ifdef SYSTEM76_EC
    if (input_disabled) {
        return false;
    }
#endif // SYSTEM76_EC

    if (!process_record_user(keycode, record)) {
        return false;
    }

    bool shifted = get_mods() & MOD_MASK_SHIFT;
    switch (keycode) {
        case QK_BOOT:
#ifdef SYSTEM76_EC
            if (record->event.pressed) {
                system76_ec_unlock();
            }
            return false;
#else  // !SYSTEM76_EC
            if (record->event.pressed) {
                rgb_matrix_disable_noeeprom();
            }
            return true;
#endif // SYSTEM76_EC
        case QK_RGB_MATRIX_VALUE_DOWN:
            if (record->event.pressed) {
                uint8_t level = rgb_matrix_get_val();
                for (uint8_t i = ARRAY_SIZE(levels); i-- > 0;) {
                    if (levels[i] < level) {
                        level = levels[i];
                        break;
                    }
                }
                set_value_all_layers(level);
            }
            return false;
        case QK_RGB_MATRIX_VALUE_UP:
            if (record->event.pressed) {
                uint8_t level = rgb_matrix_get_val();
                for (uint8_t i = 0; i < ARRAY_SIZE(levels); ++i) {
                    if (levels[i] > level) {
                        level = levels[i];
                        break;
                    }
                }
                set_value_all_layers(level);
            }
            return false;
        case QK_RGB_MATRIX_TOGGLE:
            if (record->event.pressed) {
                uint8_t level = 0;
                if (rgb_matrix_get_val() == 0) {
                    level = toggle_level;
                } else {
                    toggle_level = rgb_matrix_get_val();
                }
                set_value_all_layers(level);
            }
            return false;
        case QK_RGB_MATRIX_MODE_NEXT:
            if (record->event.pressed) {
                if (shifted) {
                    uint8_t mode = layer_rgb[0].mode - 1;
                    set_mode_default_layer((layer_rgb[0].mode == 0) ? RGB_MATRIX_EFFECT_MAX - 1 : mode);
                } else {
                    uint8_t mode = layer_rgb[0].mode + 1;
                    set_mode_default_layer((mode < RGB_MATRIX_EFFECT_MAX) ? mode : 0);
                }
            }
            return false;
        case QK_RGB_MATRIX_MODE_PREVIOUS:
            if (record->event.pressed) {
                if (!shifted) {
                    uint8_t mode = layer_rgb[0].mode - 1;
                    set_mode_default_layer((layer_rgb[0].mode == 0) ? RGB_MATRIX_EFFECT_MAX - 1 : mode);
                } else {
                    uint8_t mode = layer_rgb[0].mode + 1;
                    set_mode_default_layer((mode < RGB_MATRIX_EFFECT_MAX) ? mode : 0);
                }
            }
            return false;
#ifdef SYSTEM76_EC
        case KC_LEFT_CTRL:
            lctl_pressed = record->event.pressed;
            break;
        case KC_RIGHT_CTRL:
            rctl_pressed = record->event.pressed;
            break;
        case KC_ESCAPE:
            if (lctl_pressed && rctl_pressed) {
                if (record->event.pressed) {
                    system76_ec_unlock();
                }
                return false;
            } else {
                lctl_pressed = rctl_pressed = false;
            }
            break;
#endif // SYSTEM76_EC
    }

    return true;
}

// Layer change callback.
layer_state_t layer_state_set_kb(layer_state_t state) {
    state = layer_state_set_user(state);
    system76_ec_rgb_layer(state);

    return state;
}

void keyboard_post_init_kb(void) {
    system76_ec_rgb_layer(layer_state);

    keyboard_post_init_user();
}
