#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <purple.h>
#include "jabber.h"

#include "axc.h"
#include "libomemo.h"

#include "../src/lurch_api.h"

char * __wrap_purple_account_get_username(PurpleAccount * acc_p) {
    char * username;

    username = mock_ptr_type(char *);

    return username;
}

int __wrap_omemo_storage_user_devicelist_retrieve(const char * user, const char * db_fn, omemo_devicelist ** dl_pp) {
    omemo_devicelist * dl_p;
    dl_p = mock_ptr_type(omemo_devicelist *);
    *dl_pp = dl_p;

    int ret_val;
    ret_val = mock_type(int);

    return ret_val;
}

int __wrap_axc_get_device_id(axc_context * ctx_p, uint32_t * id_p) {
    uint32_t id;

    id = mock_type(uint32_t);

    *id_p = id;

    return EXIT_SUCCESS;
}

void __wrap_jabber_pep_publish(JabberStream * js_p, xmlnode * publish_node_p) {
    xmlnode * item_node_p = xmlnode_get_child(publish_node_p, "item");
    xmlnode * list_node_p = xmlnode_get_child(item_node_p, "list");

    xmlnode * device_node_p = xmlnode_get_child(list_node_p, "device");
    const char * device_id = xmlnode_get_attrib(device_node_p, "id");
    check_expected(device_id);

    check_expected_ptr(device_node_p->next);
}

PurpleConnection * __wrap_purple_account_get_connection(PurpleAccount * acc_p) {
    function_called();
}

int __wrap_omemo_storage_chatlist_delete(const char * chat, const char * db_fn) {
    check_expected(chat);

    int ret_val;
    ret_val = mock_type(int);
    return ret_val;
}

int __wrap_omemo_storage_chatlist_save(const char * chat, const char * db_fn) {
    check_expected(chat);

    int ret_val;
    ret_val = mock_type(int);
    return ret_val;
}

void lurch_api_id_list_handler_cb_mock(int32_t err, GList * id_list, void * user_data_p) {
    check_expected(err);

    int32_t first_id = omemo_devicelist_list_data(id_list);
    check_expected(first_id);

    int32_t second_id = omemo_devicelist_list_data(id_list->next);
    check_expected(second_id);

    GList * third_item = id_list->next->next;
    check_expected(third_item);

    check_expected(user_data_p);
}

void lurch_api_id_list_handler_cb_err_mock(int32_t err, GList * id_list, void * user_data_p) {
    check_expected(err);

    check_expected(user_data_p);
}

/**
 * Calls the supplied callback with the devicelist and the supplied user data, making sure the own ID comes first.
 */
static void test_lurch_api_id_list_handler(void ** state) {
    (void) state;

    const char * test_jid = "me-testing@test.org/resource";
    will_return(__wrap_purple_account_get_username, test_jid);

    char * devicelist = "<items node='urn:xmpp:omemo:0:devicelist'>"
                              "<item>"
                                "<list xmlns='urn:xmpp:omemo:0'>"
                                   "<device id='4223' />"
                                   "<device id='1337' />"
                                "</list>"
                              "</item>"
                            "</items>";

    omemo_devicelist * dl_p;
    omemo_devicelist_import(devicelist, test_jid, &dl_p);
    will_return(__wrap_omemo_storage_user_devicelist_retrieve, dl_p);
    will_return(__wrap_omemo_storage_user_devicelist_retrieve, EXIT_SUCCESS);

    uint32_t test_own_id = 1337;
    will_return(__wrap_axc_get_device_id, test_own_id);

    expect_value(lurch_api_id_list_handler_cb_mock, err, 0);
    expect_value(lurch_api_id_list_handler_cb_mock, first_id, test_own_id);
    expect_value(lurch_api_id_list_handler_cb_mock, second_id, 4223);
    expect_value(lurch_api_id_list_handler_cb_mock, third_item, NULL);

    char * test_user_data = "TEST USER DATA";
    expect_value(lurch_api_id_list_handler_cb_mock, user_data_p, test_user_data);

    lurch_api_id_list_handler(NULL, lurch_api_id_list_handler_cb_mock, test_user_data);
}

/**
 * When an error occurs, the supplied callback is called with the error code and the supplied user data.
 */
static void test_lurch_api_id_list_handler_error(void ** state) {
    (void) state;

    const char * test_jid = "me-testing@test.org/resource";
    will_return(__wrap_purple_account_get_username, test_jid);

    int test_errcode = -12345;
    will_return(__wrap_omemo_storage_user_devicelist_retrieve, NULL);
    will_return(__wrap_omemo_storage_user_devicelist_retrieve, test_errcode);

    char * test_user_data = "TEST USER DATA";
    expect_value(lurch_api_id_list_handler_cb_err_mock, err, test_errcode);
    expect_value(lurch_api_id_list_handler_cb_err_mock, user_data_p, test_user_data);

    lurch_api_id_list_handler(NULL, lurch_api_id_list_handler_cb_err_mock, test_user_data);
}

static void lurch_api_id_remove_handler_cb_mock(int32_t err, void * user_data_p) {
    check_expected(err);
    check_expected(user_data_p);
}

/**
 * Success case for removing a device ID from the own device list.
 */
static void test_lurch_api_id_remove_handler(void ** state) {
    (void) state;

    const char * test_jid = "me-testing@test.org/resource";
    will_return(__wrap_purple_account_get_username, test_jid);

    char * devicelist = "<items node='urn:xmpp:omemo:0:devicelist'>"
                              "<item>"
                                "<list xmlns='urn:xmpp:omemo:0'>"
                                   "<device id='4223' />"
                                   "<device id='1337' />"
                                "</list>"
                              "</item>"
                            "</items>";

    omemo_devicelist * dl_p;
    omemo_devicelist_import(devicelist, test_jid, &dl_p);
    will_return(__wrap_omemo_storage_user_devicelist_retrieve, dl_p);
    will_return(__wrap_omemo_storage_user_devicelist_retrieve, EXIT_SUCCESS);

    expect_function_call(__wrap_purple_account_get_connection);
    expect_string(__wrap_jabber_pep_publish, device_id, "4223");
    expect_value(__wrap_jabber_pep_publish, device_node_p->next, NULL);

    char * test_user_data = "TEST USER DATA";
    expect_value(lurch_api_id_remove_handler_cb_mock, err, EXIT_SUCCESS);
    expect_value(lurch_api_id_remove_handler_cb_mock, user_data_p, test_user_data);

    lurch_api_id_remove_handler((void *) "ignored", 1337, lurch_api_id_remove_handler_cb_mock, test_user_data);
}

/**
 * Fails when the device ID to remove is not in the own devicelist, and call the callback with the correct error code.
 */
static void test_lurch_api_id_remove_handler_id_not_in_list(void ** state) {
    (void) state;

    const char * test_jid = "me-testing@test.org/resource";
    will_return(__wrap_purple_account_get_username, test_jid);

    char * devicelist = "<items node='urn:xmpp:omemo:0:devicelist'>"
                              "<item>"
                                "<list xmlns='urn:xmpp:omemo:0'>"
                                   "<device id='4223' />"
                                   "<device id='1337' />"
                                "</list>"
                              "</item>"
                            "</items>";

    omemo_devicelist * dl_p;
    omemo_devicelist_import(devicelist, test_jid, &dl_p);
    will_return(__wrap_omemo_storage_user_devicelist_retrieve, dl_p);
    will_return(__wrap_omemo_storage_user_devicelist_retrieve, EXIT_SUCCESS);

    char * test_user_data = "TEST USER DATA";
    expect_value(lurch_api_id_remove_handler_cb_mock, err, LURCH_ERR_DEVICE_NOT_IN_LIST);
    expect_value(lurch_api_id_remove_handler_cb_mock, user_data_p, test_user_data);

    lurch_api_id_remove_handler((void *) "ignored", 7331, lurch_api_id_remove_handler_cb_mock, test_user_data);
}

static void lurch_api_enable_im_handler_cb_mock(int32_t err, void * user_data_p) {
    check_expected(err);
    check_expected(user_data_p);
}

/**
 * Deletes contact from 'the list' in the OMEMO DB when the signal is received, enabling OMEMO for this contact.
 */
static void test_lurch_api_enable_im_handler(void ** state) {
    (void) state;

    const char * contact_bare_jid = "contact_bare_jid";
    const char * test_jid = "me-testing@test.org/resource";
    will_return(__wrap_purple_account_get_username, test_jid);

    expect_string(__wrap_omemo_storage_chatlist_delete, chat, contact_bare_jid);
    will_return(__wrap_omemo_storage_chatlist_delete, EXIT_SUCCESS);

    char * test_user_data = "TEST USER DATA";
    expect_value(lurch_api_enable_im_handler_cb_mock, err, EXIT_SUCCESS);
    expect_value(lurch_api_enable_im_handler_cb_mock, user_data_p, test_user_data);

    lurch_api_enable_im_handler((void *) "ignored", contact_bare_jid, lurch_api_enable_im_handler_cb_mock, test_user_data);
}

/**
 * Calls the callback with the return code in case of an error.
 */
static void test_lurch_api_enable_im_handler_err(void ** state) {
    (void) state;

    const char * contact_bare_jid = "contact_bare_jid";
    const char * test_jid = "me-testing@test.org/resource";
    will_return(__wrap_purple_account_get_username, test_jid);

    expect_string(__wrap_omemo_storage_chatlist_delete, chat, contact_bare_jid);
    will_return(__wrap_omemo_storage_chatlist_delete, EXIT_FAILURE);

    char * test_user_data = "TEST USER DATA";
    expect_value(lurch_api_enable_im_handler_cb_mock, err, EXIT_FAILURE);
    expect_value(lurch_api_enable_im_handler_cb_mock, user_data_p, test_user_data);

    lurch_api_enable_im_handler((void *) "ignored", contact_bare_jid, lurch_api_enable_im_handler_cb_mock, test_user_data);
}

static void lurch_api_disable_im_handler_cb_mock(int32_t err, void * user_data_p) {
    check_expected(err);
    check_expected(user_data_p);
}

/**
 * Saves contact to 'the list' in the OMEMO DB when the signal is received, disabling OMEMO for this contact.
 */
static void test_lurch_api_disable_im_handler(void ** state) {
    (void) state;

    const char * contact_bare_jid = "contact_bare_jid";
    const char * test_jid = "me-testing@test.org/resource";
    will_return(__wrap_purple_account_get_username, test_jid);

    expect_string(__wrap_omemo_storage_chatlist_save, chat, contact_bare_jid);
    will_return(__wrap_omemo_storage_chatlist_save, EXIT_SUCCESS);

    char * test_user_data = "TEST USER DATA";
    expect_value(lurch_api_disable_im_handler_cb_mock, err, EXIT_SUCCESS);
    expect_value(lurch_api_disable_im_handler_cb_mock, user_data_p, test_user_data);

    lurch_api_disable_im_handler((void *) "ignored", contact_bare_jid, lurch_api_disable_im_handler_cb_mock, test_user_data);
}

/**
 * Calls the callback with the return code in case of an error.
 */
static void test_lurch_api_disable_im_handler_err(void ** state) {
    (void) state;

    const char * contact_bare_jid = "contact_bare_jid";
    const char * test_jid = "me-testing@test.org/resource";
    will_return(__wrap_purple_account_get_username, test_jid);

    expect_string(__wrap_omemo_storage_chatlist_save, chat, contact_bare_jid);
    will_return(__wrap_omemo_storage_chatlist_save, 12345);

    char * test_user_data = "TEST USER DATA";
    expect_value(lurch_api_disable_im_handler_cb_mock, err, 12345);
    expect_value(lurch_api_disable_im_handler_cb_mock, user_data_p, test_user_data);

    lurch_api_disable_im_handler((void *) "ignored", contact_bare_jid, lurch_api_disable_im_handler_cb_mock, test_user_data);
}

int __wrap_axc_key_load_public_own(axc_context * ctx_p, axc_buf ** pubkey_data_pp) {
    void * pubkey_data_p;
    int ret_val;

    pubkey_data_p = mock_ptr_type(void *);
    *pubkey_data_pp = pubkey_data_p;

    ret_val = mock_type(int);
    return ret_val;
}

char * __wrap_lurch_util_fp_get_printable(axc_buf * key_buf_p) {
    function_called();

    check_expected(key_buf_p);

    char * ret_val;
    ret_val = mock_ptr_type(char *);
    return ret_val;
}

void lurch_api_fp_get_handler_cb_mock(int32_t err, const char * fp_printable, void * user_data_p) {
    check_expected(err);
    check_expected(fp_printable);
    check_expected(user_data_p);
}

/**
 * Loads the account's public identity key and returns a fingerprint suitable for display.
 */
static void test_lurch_api_fp_get_handler(void ** state) {
    (void) state;

    const char * test_jid = "me-testing@test.org/resource";
    will_return(__wrap_purple_account_get_username, test_jid);

    axc_buf * mock_key_data_buf_p = axc_buf_create(NULL, 0);
    will_return(__wrap_axc_key_load_public_own, mock_key_data_buf_p);
    will_return(__wrap_axc_key_load_public_own, EXIT_SUCCESS);

    expect_function_call(__wrap_lurch_util_fp_get_printable);
    expect_value(__wrap_lurch_util_fp_get_printable, key_buf_p, mock_key_data_buf_p);
    char * mock_fp = g_strdup("MOCK FINGERPRINT");
    will_return(__wrap_lurch_util_fp_get_printable, mock_fp);

    char * test_user_data = "TEST USER DATA";
    expect_value(lurch_api_fp_get_handler_cb_mock, err, EXIT_SUCCESS);
    expect_string(lurch_api_fp_get_handler_cb_mock, fp_printable, mock_fp);
    expect_value(lurch_api_fp_get_handler_cb_mock, user_data_p, test_user_data);

    lurch_api_fp_get_handler(NULL, lurch_api_fp_get_handler_cb_mock, test_user_data);
}

/**
 * Calls the callback with the return code in case of an error.
 */
static void test_lurch_api_fp_get_handler_err(void ** state) {
    (void) state;

    const char * test_jid = "me-testing@test.org/resource";
    will_return(__wrap_purple_account_get_username, test_jid);

    will_return(__wrap_axc_key_load_public_own, NULL);
    will_return(__wrap_axc_key_load_public_own, EXIT_FAILURE);

    char * test_user_data = "TEST USER DATA";
    expect_value(lurch_api_fp_get_handler_cb_mock, err, EXIT_FAILURE);
    expect_value(lurch_api_fp_get_handler_cb_mock, fp_printable, NULL);
    expect_value(lurch_api_fp_get_handler_cb_mock, user_data_p, test_user_data);

    lurch_api_fp_get_handler(NULL, lurch_api_fp_get_handler_cb_mock, test_user_data);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_lurch_api_id_list_handler),
        cmocka_unit_test(test_lurch_api_id_list_handler_error),
        cmocka_unit_test(test_lurch_api_id_remove_handler),
        cmocka_unit_test(test_lurch_api_id_remove_handler_id_not_in_list),
        cmocka_unit_test(test_lurch_api_enable_im_handler),
        cmocka_unit_test(test_lurch_api_enable_im_handler_err),
        cmocka_unit_test(test_lurch_api_disable_im_handler),
        cmocka_unit_test(test_lurch_api_disable_im_handler_err),
        cmocka_unit_test(test_lurch_api_fp_get_handler),
        cmocka_unit_test(test_lurch_api_fp_get_handler_err)
    };

    return cmocka_run_group_tests_name("lurch_api", tests, NULL, NULL);
}