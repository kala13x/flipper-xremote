/*
   This file was taken from the project: 
   https://github.com/DarkFlippers/unleashed-firmware

   The original project is licensed under the GNU GPLv3

   Modifications made:
   - Added function infrared_remote_button_get_furi_name()
*/

#include "infrared_remote_button.h"

#include <stdlib.h>

struct InfraredRemoteButton {
    FuriString* name;
    InfraredSignal* signal;
};

InfraredRemoteButton* infrared_remote_button_alloc() {
    InfraredRemoteButton* button = malloc(sizeof(InfraredRemoteButton));
    button->name = furi_string_alloc();
    button->signal = infrared_signal_alloc();
    return button;
}

void infrared_remote_button_free(InfraredRemoteButton* button) {
    furi_string_free(button->name);
    infrared_signal_free(button->signal);
    free(button);
}

void infrared_remote_button_set_name(InfraredRemoteButton* button, const char* name) {
    furi_string_set(button->name, name);
}

const char* infrared_remote_button_get_name(InfraredRemoteButton* button) {
    return furi_string_get_cstr(button->name);
}

FuriString* infrared_remote_button_get_furi_name(InfraredRemoteButton* button) {
    return button->name;
}

void infrared_remote_button_set_signal(InfraredRemoteButton* button, InfraredSignal* signal) {
    infrared_signal_set_signal(button->signal, signal);
}

InfraredSignal* infrared_remote_button_get_signal(InfraredRemoteButton* button) {
    return button->signal;
}
