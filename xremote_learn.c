/*!
 *  @file flipper-xremote/xremote_learn.c
    @license This project is released under the GNU GPLv3 License
 *  @copyright (c) 2023 Sandro Kalatozishvili (s.kalatoz@gmail.com)
 *
 * @brief Functionality to read and store input from new remote.
 */

#include "xremote_learn.h"
#include "views/xremote_learn_view.h"

#define XREMOTE_TEXT_MAX    128

struct XRemoteLearnContext {
    /* XRemote context */
    XRemoteSignalReceiver* ir_receiver;
    XRemoteAppContext* app_ctx;
    XRemoteView* signal_view;

    /* Main infrared app context */
    InfraredRemote* ir_remote;
    InfraredSignal* ir_signal;

    /* User interactions */
    TextInput* text_input;
    DialogEx* dialog_ex;

    /* User context and clear callback */
    XRemoteClearCallback on_clear;
    void* context;

    /* Private control flags */
    char text_store[XREMOTE_TEXT_MAX + 1];
    uint8_t current_button;
    bool finish_learning;
    bool stop_receiver;
};

static uint32_t xremote_learn_view_exit_callback(void* context)
{
    UNUSED(context);
    return XRemoteViewSubmenu;
}

static uint32_t xremote_learn_child_view_exit_callback(void* context)
{
    UNUSED(context);
    return XRemoteViewLearn;
}

static void xremote_learn_send_event(XRemoteLearnContext* learn_ctx, XRemoteEvent event)
{
    xremote_app_assert_void(learn_ctx);
    ViewDispatcher* view_disp = learn_ctx->app_ctx->view_dispatcher;
    view_dispatcher_send_custom_event(view_disp, event);
}

static void xremote_learn_switch_to_view(XRemoteLearnContext* learn_ctx, XRemoteViewID view_id)
{
    xremote_app_assert_void(learn_ctx);
    ViewDispatcher* view_disp = learn_ctx->app_ctx->view_dispatcher;
    view_dispatcher_switch_to_view(view_disp, view_id);
}

static void xremote_learn_context_rx_stop(XRemoteLearnContext *learn_ctx)
{
    xremote_app_assert_void(learn_ctx);
    learn_ctx->stop_receiver = true;
    xremote_signal_receiver_stop(learn_ctx->ir_receiver);
}

static void xremote_learn_context_rx_start(XRemoteLearnContext *learn_ctx)
{
    xremote_app_assert_void(learn_ctx);
    learn_ctx->stop_receiver = false;
    xremote_signal_receiver_start(learn_ctx->ir_receiver);
}

void xremote_learn_context_ask_finish(XRemoteLearnContext *learn_ctx)
{
    xremote_app_assert_void(learn_ctx);
    learn_ctx->finish_learning = true;
    xremote_learn_send_event(learn_ctx, XRemoteEventSignalFinish);
}

const char* xremote_learn_get_curr_button_name(XRemoteLearnContext *learn_ctx)
{
    xremote_app_assert(learn_ctx, NULL);
    return xremote_button_get_name(learn_ctx->current_button);
}

int xremote_learn_get_curr_button_index(XRemoteLearnContext *learn_ctx)
{
    xremote_app_assert(learn_ctx, -1);
    return learn_ctx->current_button;
}

InfraredRemote* xremote_learn_get_ir_remote(XRemoteLearnContext *learn_ctx)
{
    xremote_app_assert(learn_ctx, NULL);
    return learn_ctx->ir_remote;
}

InfraredSignal* xremote_learn_get_ir_signal(XRemoteLearnContext *learn_ctx)
{
    xremote_app_assert(learn_ctx, NULL);
    return learn_ctx->ir_signal;
}

XRemoteSignalReceiver* xremote_learn_get_ir_receiver(XRemoteLearnContext *learn_ctx)
{
    xremote_app_assert(learn_ctx, NULL);
    return learn_ctx->ir_receiver;
}

XRemoteAppContext* xremote_learn_get_app_context(XRemoteLearnContext *learn_ctx)
{
    xremote_app_assert(learn_ctx, NULL);
    return learn_ctx->app_ctx;
}

static void xremote_learn_text_input_callback(void* context)
{
    xremote_app_assert_void(context);
    XRemoteLearnContext *learn_ctx = context;

    // TODO: save file here

    xremote_learn_send_event(learn_ctx, XRemoteEventSignalExit);
}

static void xremote_learn_dialog_exit_callback(DialogExResult result, void* context)
{
    XRemoteLearnContext* learn_ctx = (XRemoteLearnContext*)context;
    xremote_learn_switch_to_view(learn_ctx, XRemoteViewSubmenu);

    if (result == DialogExResultLeft)
        xremote_learn_send_event(learn_ctx, XRemoteEventSignalExit);
    else if (result == DialogExResultRight)
        xremote_learn_send_event(learn_ctx, XRemoteEventSignalRetry);
    else if (result == DialogExResultCenter)
        xremote_learn_send_event(learn_ctx, XRemoteEventSignalFinish);
}

static void xremote_learn_exit_dialog_free(XRemoteLearnContext *learn_ctx)
{
    xremote_app_assert_void(learn_ctx);
    xremote_app_assert_void(learn_ctx->dialog_ex);

    ViewDispatcher* view_disp = learn_ctx->app_ctx->view_dispatcher;
    view_dispatcher_remove_view(view_disp, XRemoteViewDialogExit);

    dialog_ex_free(learn_ctx->dialog_ex);
    learn_ctx->dialog_ex = NULL;
}

static void xremote_learn_exit_dialog_alloc(XRemoteLearnContext *learn_ctx, const char *header, DialogExResultCallback callback)
{
    xremote_app_assert_void(learn_ctx);
    xremote_learn_exit_dialog_free(learn_ctx);

    ViewDispatcher *view_disp = learn_ctx->app_ctx->view_dispatcher;
    const char *dialog_text = "All unsaved data\nwill be lost!";

    learn_ctx->dialog_ex = dialog_ex_alloc();
    View *view = dialog_ex_get_view(learn_ctx->dialog_ex);
    view_dispatcher_add_view(view_disp, XRemoteViewDialogExit, view);

    dialog_ex_set_header(learn_ctx->dialog_ex, header, 64, 11, AlignCenter, AlignTop);
    dialog_ex_set_text(learn_ctx->dialog_ex, dialog_text, 64, 25, AlignCenter, AlignTop);
    dialog_ex_set_icon(learn_ctx->dialog_ex, 0, 0, NULL);

    dialog_ex_set_left_button_text(learn_ctx->dialog_ex, "Exit");
    dialog_ex_set_center_button_text(learn_ctx->dialog_ex, "Save");
    dialog_ex_set_right_button_text(learn_ctx->dialog_ex, "Stay");

    dialog_ex_set_result_callback(learn_ctx->dialog_ex, callback);
    dialog_ex_set_context(learn_ctx->dialog_ex, learn_ctx);
}

static void xremote_learn_exit_dialog_ask(XRemoteLearnContext *learn_ctx)
{
    xremote_app_assert_void(learn_ctx);
    xremote_learn_exit_dialog_alloc(learn_ctx,
        "Exit to XRemote Menu?",
        xremote_learn_dialog_exit_callback);

    xremote_learn_switch_to_view(learn_ctx, XRemoteViewDialogExit);

    // TODO: check and exit
    //xremote_learn_switch_to_view(learn_ctx, XRemoteViewSubmenu);
}

static void xremote_learn_store_database_to_file(XRemoteLearnContext *learn_ctx)
{
    xremote_app_assert_void(learn_ctx);
    xremote_app_assert_void(learn_ctx->text_input);

    snprintf(learn_ctx->text_store, XREMOTE_TEXT_MAX, "Remote_");
    text_input_set_header_text(learn_ctx->text_input, "Name new remote");

    text_input_set_result_callback(
        learn_ctx->text_input,
        xremote_learn_text_input_callback,
        learn_ctx,
        learn_ctx->text_store,
        XREMOTE_TEXT_MAX,
        true);

    ViewDispatcher* view_disp = learn_ctx->app_ctx->view_dispatcher;
    view_dispatcher_switch_to_view(view_disp, XRemoteViewTextInput);
}

static void xremote_learn_signal_callback(void *context, InfraredSignal* signal)
{
    XRemoteLearnContext* learn_ctx = context;
    xremote_app_assert_void(!learn_ctx->stop_receiver);
    xremote_app_assert_void(!learn_ctx->finish_learning);
    learn_ctx->stop_receiver = true;

    infrared_signal_set_signal(learn_ctx->ir_signal, signal);
    xremote_learn_send_event(learn_ctx, XRemoteEventSignalReceived);
}

static bool xremote_learn_custom_event_callback(void* context, uint32_t event)
{
    xremote_app_assert(context, false);
    XRemoteLearnContext *learn_ctx = context;

    /* If finishing ignore every signal event except XRemoteEventSignalFinish */
    if (learn_ctx->finish_learning && event != XRemoteEventSignalFinish) return true;

    if (event == XRemoteEventSignalReceived)
    {
        xremote_learn_context_rx_stop(learn_ctx);
        xremote_learn_switch_to_view(learn_ctx, XRemoteViewSignal);
    }
    else if (event == XRemoteEventSignalSave)
    {
        learn_ctx->current_button++;
        if (learn_ctx->current_button >= XREMOTE_BUTTON_COUNT)
        {
            xremote_learn_context_rx_stop(learn_ctx);
            xremote_learn_context_ask_finish(learn_ctx);
            return true;
        }

        xremote_learn_context_rx_start(learn_ctx);
        xremote_learn_switch_to_view(learn_ctx, XRemoteViewLearn);
    }
    else if (event == XRemoteEventSignalSkip)
    {
        learn_ctx->current_button++;
        if (learn_ctx->current_button >= XREMOTE_BUTTON_COUNT)
        {
            xremote_learn_context_rx_stop(learn_ctx);
            xremote_learn_context_ask_finish(learn_ctx);
            return true;
        }

        xremote_learn_context_rx_start(learn_ctx);
        xremote_learn_switch_to_view(learn_ctx, XRemoteViewLearn);
    }
    else if (event == XRemoteEventSignalRetry)
    {
        xremote_learn_context_rx_start(learn_ctx);
        xremote_learn_switch_to_view(learn_ctx, XRemoteViewLearn);
    }
    else if (event == XRemoteEventSignalFinish)
    {
        xremote_learn_context_rx_stop(learn_ctx);
        xremote_learn_store_database_to_file(learn_ctx);
    }
    else if (event == XRemoteEventSignalAskExit)
    {
        xremote_learn_context_rx_stop(learn_ctx);
        xremote_learn_exit_dialog_ask(learn_ctx);
    }
    else if (event == XRemoteEventSignalExit)
    {
        xremote_learn_context_rx_stop(learn_ctx);
        xremote_learn_switch_to_view(learn_ctx, XRemoteViewSubmenu);
    }

    return true;
}

static XRemoteLearnContext* xremote_learn_context_alloc(XRemoteAppContext* app_ctx)
{
    XRemoteLearnContext *learn_ctx = malloc(sizeof(XRemoteLearnContext));
    learn_ctx->ir_signal = infrared_signal_alloc();

    learn_ctx->finish_learning = false;
    learn_ctx->stop_receiver = false;
    learn_ctx->current_button = 0;
    learn_ctx->text_store[0] = 0;
    learn_ctx->dialog_ex = NULL;
    learn_ctx->app_ctx = app_ctx;

    learn_ctx->signal_view = xremote_learn_success_view_alloc(app_ctx, learn_ctx);
    View* view = xremote_view_get_view(learn_ctx->signal_view);
    view_set_previous_callback(view, xremote_learn_child_view_exit_callback);
    view_dispatcher_add_view(app_ctx->view_dispatcher, XRemoteViewSignal, view);

    learn_ctx->text_input = text_input_alloc();
    view = text_input_get_view(learn_ctx->text_input);
    view_set_previous_callback(view, xremote_learn_child_view_exit_callback);
    view_dispatcher_add_view(app_ctx->view_dispatcher, XRemoteViewTextInput, view);

    view_dispatcher_set_custom_event_callback(app_ctx->view_dispatcher, xremote_learn_custom_event_callback);
    view_dispatcher_set_event_callback_context(app_ctx->view_dispatcher, learn_ctx);

    learn_ctx->ir_receiver = xremote_signal_receiver_alloc(app_ctx);
    xremote_signal_receiver_set_context(learn_ctx->ir_receiver, learn_ctx, NULL);
    xremote_signal_receiver_set_rx_callback(learn_ctx->ir_receiver, xremote_learn_signal_callback);

    return learn_ctx;
}

static void xremote_learn_context_free(XRemoteLearnContext* learn_ctx)
{
    xremote_app_assert_void(learn_ctx);
    xremote_signal_receiver_stop(learn_ctx->ir_receiver);
    xremote_learn_exit_dialog_free(learn_ctx);

    ViewDispatcher* view_disp = learn_ctx->app_ctx->view_dispatcher;
    view_dispatcher_remove_view(view_disp, XRemoteViewTextInput);
    text_input_free(learn_ctx->text_input);

    view_dispatcher_remove_view(view_disp, XRemoteViewSignal);
    xremote_view_free(learn_ctx->signal_view);

    xremote_signal_receiver_free(learn_ctx->ir_receiver);
    infrared_signal_free(learn_ctx->ir_signal);
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
    app->view_id = XRemoteViewLearn;

    XRemoteLearnContext* learn = xremote_learn_context_alloc(app_ctx);
    app->view_ctx = xremote_learn_view_alloc(app->app_ctx, learn);
    View* view = xremote_view_get_view(app->view_ctx);

    ViewDispatcher* view_disp = app_ctx->view_dispatcher;
    view_dispatcher_add_view(view_disp, app->view_id, view);

    xremote_app_view_set_previous_callback(app, xremote_learn_view_exit_callback);
    xremote_app_set_view_context(app, learn, xremote_learn_context_clear_callback);

    xremote_signal_receiver_start(learn->ir_receiver);
    return app;
}
