/*!
 *  @file flipper-xremote/views/xremote_learn_view.c
    @license This project is released under the GNU GPLv3 License
 *  @copyright (c) 2023 Sandro Kalatozishvili (s.kalatoz@gmail.com)
 *
 * @brief Learn new remote page view components and functionality.
 */

#include "xremote_learn_view.h"
#include "../xremote_learn.h"
#include "../xremote_app.h"

static void xremote_learn_view_draw_callback(Canvas* canvas, void* context)
{
    furi_assert(context);
    XRemoteViewModel* model = context;
    XRemoteAppContext *app_ctx = model->context;

    ViewOrientation orientation = app_ctx->app_settings->orientation;
    uint64_t x = orientation == ViewOrientationVertical ? 40 : 15;

    xremote_canvas_draw_header(canvas, orientation, "Learn");
    canvas_set_font(canvas, FontSecondary);
    elements_multiline_text_aligned(canvas, 0, x, AlignLeft, AlignTop, 
        "Press any\nbutton\non remote");
    xremote_canvas_draw_exit_footer(canvas, orientation, "Press to exit");
}

XRemoteView* xremote_learn_view_alloc(void* app_ctx)
{
    XRemoteView *view = xremote_view_alloc(app_ctx,
        NULL, xremote_learn_view_draw_callback);

    with_view_model(
        xremote_view_get_view(view),
        XRemoteViewModel* model,
        { model->context = app_ctx; },
        true
    );

    return view;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Learn alpha implementation
//////////////////////////////////////////////////////////////////////////////////////////

static void xremote_signal_view_draw_callback(Canvas* canvas, void* context)
{
    furi_assert(context);
    XRemoteViewModel* model = context;
    XRemoteLearnContext* learn_ctx = model->context;
    XRemoteAppContext* app_ctx = learn_ctx->app_ctx;
    ViewOrientation orientation = app_ctx->app_settings->orientation;

    xremote_canvas_draw_header(canvas, orientation, "RX signal");
    canvas_set_font(canvas, FontSecondary);

    InfraredMessage* message = infrared_signal_get_message(learn_ctx->rx_signal);
    const char *infrared_protocol = infrared_get_protocol_name(message->protocol);
    uint64_t y = orientation == ViewOrientationVertical ? 30 : 10;

    char signal[128];
    snprintf(signal, sizeof(signal),
        "Protocol: %s\n"
        "Address: 0x%lX\n"
        "Command: 0x%lX\n"
        "Repeat: %s\r\n",
        infrared_protocol,
        message->address,
        message->command,
        message->repeat ? "Yes" : "No");

    elements_multiline_text_aligned(canvas, 0, y, AlignLeft, AlignTop, signal);

    if (orientation == ViewOrientationHorizontal)
        xremote_canvas_draw_button_wide(canvas, model->left_pressed, 68, 35, "Retry", XRemoteIconArrowLeft);
    else
        xremote_canvas_draw_button_wide(canvas, model->left_pressed, 0, 100, "Retry", XRemoteIconArrowLeft);

    xremote_canvas_draw_exit_footer(canvas, orientation, "Press to exit");
}

static void xremote_signal_view_process(XRemoteView* view, InputEvent* event)
{
    with_view_model(
        xremote_view_get_view(view),
        XRemoteViewModel* model,
        {
            XRemoteLearnContext* learn_ctx = xremote_view_get_context(view);
            XRemoteAppExit exit = learn_ctx->app_ctx->app_settings->exit_behavior;
            model->context = learn_ctx;

            if (event->type == InputTypePress)
            {
                if (event->key == InputKeyUp)
                {
                    model->up_pressed = true;
                }
                else if (event->key == InputKeyDown)
                {
                    model->down_pressed = true;
                }
                else if (event->key == InputKeyLeft)
                {
                    model->left_pressed = true;
                    xremote_signal_receiver_resume(learn_ctx->rx_ctx);

                    ViewDispatcher* view_disp = learn_ctx->app_ctx->view_dispatcher;
                    view_dispatcher_switch_to_view(view_disp, XRemoteViewLearn);
                }
                else if (event->key == InputKeyRight)
                {
                    model->right_pressed = true;
                }
                else if (event->key == InputKeyOk)
                {
                    model->ok_pressed = true;
                }
            }
            else if (event->type == InputTypeShort &&
                    event->key == InputKeyBack &&
                    exit == XRemoteAppExitHold)
            {
                model->back_pressed = true;
            }
            else if (event->type == InputTypeLong &&
                    event->key == InputKeyBack &&
                    exit == XRemoteAppExitPress)
            {
                model->back_pressed = true;
            }
            else if (event->type == InputTypeRelease)
            {
                if (event->key == InputKeyUp) model->up_pressed = false;
                else if (event->key == InputKeyDown) model->down_pressed = false;
                else if (event->key == InputKeyLeft) model->left_pressed = false;
                else if (event->key == InputKeyRight) model->right_pressed = false;
                else if (event->key == InputKeyOk) model->ok_pressed = false;
                else if (event->key == InputKeyBack) model->back_pressed = false;
            }
        },
        true);
}


static bool xremote_signal_view_input_callback(InputEvent* event, void* context)
{
    furi_assert(context);
    XRemoteView* view = (XRemoteView*)context;
    XRemoteAppContext* app_ctx = xremote_view_get_app_context(view);
    XRemoteAppExit exit = app_ctx->app_settings->exit_behavior;

    if (event->key == InputKeyBack &&
        event->type == InputTypeShort &&
        exit == XRemoteAppExitPress) return false;
    else if (event->key == InputKeyBack &&
        event->type == InputTypeLong &&
        exit == XRemoteAppExitHold) return false;

    xremote_signal_view_process(view, event);
    return true;
}

XRemoteView* xremote_signal_view_alloc(void* app_ctx, void *learn_ctx)
{
    XRemoteView *view = xremote_view_alloc(app_ctx,
        xremote_signal_view_input_callback,
        xremote_signal_view_draw_callback);
    xremote_view_set_context(view, learn_ctx, NULL);

    with_view_model(
        xremote_view_get_view(view),
        XRemoteViewModel* model,
        {
            model->context = learn_ctx;
            model->up_pressed = false;
            model->down_pressed = false;
            model->left_pressed = false;
            model->right_pressed = false;
            model->back_pressed = false;
            model->ok_pressed = false;
        },
        true
    );

    return view;
}

//////////////////////////////////////////////////////////////////////////////////////////
// END OF: Learn alpha implementation
//////////////////////////////////////////////////////////////////////////////////////////