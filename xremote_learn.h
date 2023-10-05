/*!
 *  @file flipper-xremote/xremote_learn.h
    @license This project is released under the GNU GPLv3 License
 *  @copyright (c) 2023 Sandro Kalatozishvili (s.kalatoz@gmail.com)
 *
 * @brief Functionality to read and store input from new remote.
 */

#pragma once

#include "xremote_app.h"
#include "xremote_signal.h"

typedef struct {
    XRemoteSignalReceiver* ir_receiver;
    XRemoteClearCallback on_clear;
    XRemoteAppContext* app_ctx;
    InfraredSignal *rx_signal;
    XRemoteView* signal_view;
    bool processing_signal;
    bool finish_learning;
    uint8_t current_button;
    void* context;
} XRemoteLearnContext;

XRemoteApp* xremote_learn_alloc(XRemoteAppContext* app_ctx);
