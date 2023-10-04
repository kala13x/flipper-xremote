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
    uint8_t x = orientation == ViewOrientationVertical ? 40 : 15;

    xremote_canvas_draw_header(canvas, orientation, "Learn");
    canvas_set_font(canvas, FontSecondary);
    elements_multiline_text_aligned(canvas, 0, x, AlignLeft, AlignTop, 
        "Press any\nbutton\non remote");
    xremote_canvas_draw_exit_footer(canvas, orientation, "Press to exit");
}

static void xremote_learn_success_view_draw_callback(Canvas* canvas, void* context)
{
    furi_assert(context);
    XRemoteViewModel* model = context;
    XRemoteLearnContext* learn_ctx = model->context;
    XRemoteAppContext* app_ctx = learn_ctx->app_ctx;

    xremote_canvas_draw_header(canvas, app_ctx->app_settings->orientation, NULL);
    const char *button_name = "TODO"; // temporary
    char signal_info[128];

    if (infrared_signal_is_raw(learn_ctx->rx_signal))
    {
        InfraredRawSignal* raw = infrared_signal_get_raw_signal(learn_ctx->rx_signal);

        snprintf(signal_info, sizeof(signal_info),
            "Name: %s\n"
            "Type: RAW\n"
            "T-Size: %u\n"
            "D-Cycle: %.2f\n",
            button_name,
            raw->timings_size,
            (double)raw->duty_cycle);
    }
    else
    {
        InfraredMessage* message = infrared_signal_get_message(learn_ctx->rx_signal);
        const char *infrared_protocol = infrared_get_protocol_name(message->protocol);

        snprintf(signal_info, sizeof(signal_info),
            "Name: %s\n"
            "Proto: %s\n"
            "Addr: 0x%lX\n"
            "Cmd: 0x%lX\n",
            button_name,
            infrared_protocol,
            message->address,
            message->command);
    }

    if (app_ctx->app_settings->orientation == ViewOrientationHorizontal)
    {
        canvas_draw_str_aligned(canvas, 0, 0, AlignLeft, AlignTop, "Received signal");
        elements_multiline_text_aligned(canvas, 0, 16, AlignLeft, AlignTop, signal_info);
        xremote_canvas_draw_button_wide(canvas, model->ok_pressed, 68, 12, "Finish", XRemoteIconEnter);
        xremote_canvas_draw_button_wide(canvas, model->right_pressed, 68, 30, "Next", XRemoteIconArrowRight);
        xremote_canvas_draw_button_wide(canvas, model->back_pressed, 68, 48, "Retry", XRemoteIconBack);
    }
    else
    {
        canvas_draw_str_aligned(canvas, 0, 12, AlignLeft, AlignTop, "Received signal");
        elements_multiline_text_aligned(canvas, 0, 28, AlignLeft, AlignTop, signal_info);
        xremote_canvas_draw_button_wide(canvas, model->ok_pressed, 0, 76, "Finish", XRemoteIconEnter);
        xremote_canvas_draw_button_wide(canvas, model->right_pressed, 0, 94, "Next", XRemoteIconArrowRight);
        xremote_canvas_draw_button_wide(canvas, model->back_pressed, 0, 112, "Retry", XRemoteIconBack);
    }
}

static void xremote_learn_success_view_process(XRemoteView* view, InputEvent* event)
{
    with_view_model(
        xremote_view_get_view(view),
        XRemoteViewModel* model,
        {
            model->context = xremote_view_get_context(view);

            if (event->type == InputTypePress)
            {
                if (event->key == InputKeyOk)
                {
                    model->ok_pressed = true;
                }
                else if (event->key == InputKeyRight)
                {
                    model->right_pressed = true;
                }
            }
            else if (event->type == InputTypeRelease)
            {
                if (event->key == InputKeyRight) model->right_pressed = false;
                else if (event->key == InputKeyOk) model->ok_pressed = false;
            }
        },
        true);
}

static bool xremote_learn_success_view_input_callback(InputEvent* event, void* context)
{
    furi_assert(context);
    XRemoteView* view = (XRemoteView*)context;

    if (event->key == InputKeyBack)
    {
        XRemoteAppContext* app_ctx = xremote_view_get_app_context(view);
        view_dispatcher_send_custom_event(app_ctx->view_dispatcher, XRemoteEventSignalRetry);

        return true;
    }

    xremote_learn_success_view_process(view, event);
    return true;
}

XRemoteView* xremote_learn_success_view_alloc(void* app_ctx, void *learn_ctx)
{
    XRemoteView *view = xremote_view_alloc(app_ctx,
        xremote_learn_success_view_input_callback,
        xremote_learn_success_view_draw_callback);
    xremote_view_set_context(view, learn_ctx, NULL);

    with_view_model(
        xremote_view_get_view(view),
        XRemoteViewModel* model,
        {
            model->context = learn_ctx;
            model->right_pressed = false;
            model->back_pressed = false;
            model->ok_pressed = false;
        },
        true
    );

    return view;
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
