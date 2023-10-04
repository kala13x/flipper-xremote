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
    XRemoteAppContext* app_ctx = learn_ctx->app_ctx;
    ViewDispatcher* view_disp = app_ctx->view_dispatcher;
    learn_ctx->rx_signal = signal;

    xremote_signal_receiver_pause(learn_ctx->rx_ctx);
    view_dispatcher_switch_to_view(view_disp, XRemoteViewSignal);
}

static XRemoteLearnContext* xremote_learn_context_alloc(XRemoteAppContext* app_ctx)
{
    XRemoteLearnContext *learn_ctx = malloc(sizeof(XRemoteLearnContext));
    learn_ctx->signal_view = xremote_learn_success_view_alloc(app_ctx, learn_ctx);
    learn_ctx->rx_ctx = xremote_signal_receiver_alloc(app_ctx);
    learn_ctx->app_ctx = app_ctx;
    learn_ctx->rx_signal = NULL;

    View* view = xremote_view_get_view(learn_ctx->signal_view);
    view_set_previous_callback(view, xremote_learn_success_view_exit_callback);
    view_dispatcher_add_view(app_ctx->view_dispatcher, XRemoteViewSignal, view);

    xremote_signal_receiver_set_context(learn_ctx->rx_ctx, learn_ctx, NULL);
    xremote_signal_receiver_set_rx_callback(learn_ctx->rx_ctx, xremote_learn_signal_callback);

    return learn_ctx;
}

static void xremote_learn_context_free(XRemoteLearnContext* learn_ctx)
{
    xremote_app_assert_void(learn_ctx);
    ViewDispatcher* view_disp = learn_ctx->app_ctx->view_dispatcher;
    view_dispatcher_remove_view(view_disp, XRemoteViewSignal);

    xremote_signal_receiver_free(learn_ctx->rx_ctx);
    xremote_view_free(learn_ctx->signal_view);
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

    xremote_signal_receiver_start(learn->rx_ctx);
    return app;
}
