/*!
 *  @file flipper-xremote/xremote_edit.c
    @license This project is released under the GNU GPLv3 License
 *  @copyright (c) 2023 Sandro Kalatozishvili (s.kalatoz@gmail.com)
 *
 * @brief Edit menu for XRemote custom layout buttons.
 */

#include "xremote_edit.h"

typedef struct {
    VariableItemList* item_list;
    XRemoteAppButtons* buttons;
} XRemoteEditContext;

#define XREMOTE_ORIENTATION_TEXT "Orientation"
#define XREMOTE_ORIENTATION_MAX 2

#define XREMOTE_EXIT_BEHAVIOR_TEXT "Exit Apps"
#define XREMOTE_EXIT_BEHAVIOR_MAX 2

#define XREMOTE_REPEAT_TEXT "IR Msg Repeat"
#define XREMOTE_REPEAT_MAX 128

static uint32_t xremote_edit_view_exit_callback(void* context) {
    UNUSED(context);
    return XRemoteViewIRSubmenu;
}

static void xremote_edit_buttons_store(XRemoteAppButtons* buttons) {
    XRemoteAppContext* app_ctx = buttons->app_ctx;
    infrared_remote_store(buttons->remote);
    xremote_app_extension_store(buttons, app_ctx->file_path);
}

static void xremote_edit_up_press_changed(VariableItem* item) {
    XRemoteEditContext* ctx = variable_item_get_context(item);
    XRemoteAppButtons* buttons = ctx->buttons;

    int button_index = variable_item_get_current_value_index(item);
    const char* button_name = xremote_button_get_name(button_index);

    furi_string_set_str(buttons->custom_up, button_name);
    variable_item_set_current_value_text(item, button_name);
    xremote_edit_buttons_store(buttons);
}

static XRemoteEditContext* xremote_edit_context_alloc(XRemoteAppButtons* buttons) {
    XRemoteEditContext* context = malloc(sizeof(XRemoteEditContext));
    context->item_list = variable_item_list_alloc();

    XRemoteAppContext* app_ctx = buttons->app_ctx;
    context->buttons = buttons;

    /* Configure variable item list view */
    View* view = variable_item_list_get_view(context->item_list);
    view_set_previous_callback(view, xremote_edit_view_exit_callback);
    view_dispatcher_add_view(app_ctx->view_dispatcher, XRemoteViewIRCustomEditPage, view);

    /* Add custom_up to variable item list */
    VariableItem* item = variable_item_list_add(
        context->item_list,
        "Up press",
        XREMOTE_BUTTON_COUNT,
        xremote_edit_up_press_changed,
        context);

    /* Get button name and index */
    const char* button_name = furi_string_get_cstr(buttons->custom_up);
    uint32_t button_index = xremote_button_get_index(button_name);

    /* Set button name and index to the list item */
    variable_item_set_current_value_index(item, button_index);
    variable_item_set_current_value_text(item, button_name);

    return context;
}

void xremote_edit_context_clear_callback(void* context) {
    XRemoteEditContext* ctx = (XRemoteEditContext*)context;
    variable_item_list_free(ctx->item_list);
}

void xremote_edit_view_alloc(XRemoteApp* app, uint32_t view_id, XRemoteAppButtons* buttons) {
    xremote_app_view_free(app);
    XRemoteView* remote_view = xremote_view_alloc_empty();
    XRemoteEditContext* context = xremote_edit_context_alloc(buttons);

    xremote_view_set_app_context(remote_view, buttons->app_ctx);
    xremote_view_set_view(remote_view, variable_item_list_get_view(context->item_list));
    xremote_view_set_context(remote_view, context, xremote_edit_context_clear_callback);

    app->view_ctx = remote_view;
    app->view_id = view_id;
}
