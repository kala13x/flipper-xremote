/*!
 *  @file flipper-xremote/xremote_learn.c
    @license This project is released under the GNU GPLv3 License
 *  @copyright (c) 2023 Sandro Kalatozishvili (s.kalatoz@gmail.com)
 *
 * @brief Functionality to read and store input from new remote.
 */

#include "xremote_learn.h"
#include "views/xremote_learn_view.h"

static uint32_t xremote_learn_view_exit_callback(void* context)
{
    UNUSED(context);
    return XRemoteViewSubmenu;
}

static uint32_t xremote_learn_success_view_exit_callback(void* context)
{
    UNUSED(context);
    return XRemoteViewLearn;
}

static void xremote_learn_signal_callback(void *context, InfraredSignal* signal)
{
    XRemoteLearnContext* learn_ctx = context;
    if (learn_ctx->processing_signal) return;

    infrared_signal_set_signal(learn_ctx->rx_signal, signal);
    learn_ctx->processing_signal = true;

    ViewDispatcher* view_disp = learn_ctx->app_ctx->view_dispatcher;
    view_dispatcher_send_custom_event(view_disp, XRemoteEventSignalReceived);
}

static bool xremote_learn_custom_event_callback(void* context, uint32_t event)
{
    XRemoteLearnContext *learn_ctx = context;
    ViewDispatcher* view_disp = learn_ctx->app_ctx->view_dispatcher;

    if (event == XRemoteEventSignalReceived)
    {
        xremote_signal_receiver_stop(learn_ctx->ir_receiver);
        view_dispatcher_switch_to_view(view_disp, XRemoteViewSignal);
    }
    else if (event == XRemoteEventSignalSaved)
    {
        learn_ctx->processing_signal = false;
        xremote_signal_receiver_start(learn_ctx->ir_receiver);
        view_dispatcher_switch_to_view(view_disp, XRemoteViewLearn);
    }
    else if (event == XRemoteEventSignalRetry)
    {
        learn_ctx->processing_signal = false;
        xremote_signal_receiver_start(learn_ctx->ir_receiver);
        view_dispatcher_switch_to_view(view_disp, XRemoteViewLearn);
    }

    return true;
}

static XRemoteLearnContext* xremote_learn_context_alloc(XRemoteAppContext* app_ctx)
{
    XRemoteLearnContext *learn_ctx = malloc(sizeof(XRemoteLearnContext));
    learn_ctx->signal_view = xremote_learn_success_view_alloc(app_ctx, learn_ctx);
    learn_ctx->ir_receiver = xremote_signal_receiver_alloc(app_ctx);
    learn_ctx->rx_signal = infrared_signal_alloc();
    learn_ctx->processing_signal = false;
    learn_ctx->app_ctx = app_ctx;

    View* view = xremote_view_get_view(learn_ctx->signal_view);
    view_set_previous_callback(view, xremote_learn_success_view_exit_callback);
    view_dispatcher_add_view(app_ctx->view_dispatcher, XRemoteViewSignal, view);

    view_dispatcher_set_custom_event_callback(app_ctx->view_dispatcher, xremote_learn_custom_event_callback);
    view_dispatcher_set_event_callback_context(app_ctx->view_dispatcher, learn_ctx);

    xremote_signal_receiver_set_context(learn_ctx->ir_receiver, learn_ctx, NULL);
    xremote_signal_receiver_set_rx_callback(learn_ctx->ir_receiver, xremote_learn_signal_callback);

    return learn_ctx;
}

static void xremote_learn_context_free(XRemoteLearnContext* learn_ctx)
{
    xremote_app_assert_void(learn_ctx);
    ViewDispatcher* view_disp = learn_ctx->app_ctx->view_dispatcher;
    view_dispatcher_remove_view(view_disp, XRemoteViewSignal);

    xremote_signal_receiver_free(learn_ctx->ir_receiver);
    xremote_view_free(learn_ctx->signal_view);

    infrared_signal_free(learn_ctx->rx_signal);
    free(learn_ctx);
}

static void xremote_learn_context_clear_callback(void* context)
{
    XRemoteLearnContext *learn = context;
    xremote_learn_context_free(learn);
}

XRemoteApp* xremote_learn_alloc(XRemoteAppContext* app_ctx)
{
    XRemoteApp* app = xremote_app_alloc(app_ctx);
    xremote_app_view_alloc(app, XRemoteViewLearn, xremote_learn_view_alloc);
    xremote_app_view_set_previous_callback(app, xremote_learn_view_exit_callback);

    XRemoteLearnContext* learn = xremote_learn_context_alloc(app_ctx);
    xremote_app_set_view_context(app, learn, xremote_learn_context_clear_callback);

    xremote_signal_receiver_start(learn->ir_receiver);
    return app;
}
