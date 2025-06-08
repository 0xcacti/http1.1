#include "internal/headers/headers.h"
#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <string.h>

// Helper function to get header value (you'll need to add this to your headers.h)
const char *headers_get(headers_t *h, const char *key) {
    khint_t k = kh_get(headers, h->map, key);
    if (k == kh_end(h->map)) {
        return NULL;
    }
    return kh_val(h->map, k);
}

Test(headers, valid_single_header) {
    headers_t *headers = new_headers();
    cr_assert_not_null(headers, "Headers should not be null");

    const char *data = "Host: localhost:42069\r\n\r\n";
    parse_result_t result = parse_headers(headers, data, strlen(data));

    cr_assert_eq(result.error, NULL, "Expected no error");
    cr_assert_not_null(headers, "Headers should not be null");

    const char *host_value = headers_get(headers, "Host");
    cr_assert_str_eq(host_value, "localhost:42069");
    cr_assert_eq(result.n, 23, "Expected 23 bytes consumed");
    cr_assert_eq(result.done, false, "Expected done to be false");

    free_headers(headers);
}

Test(headers, valid_single_header_with_extra_whitespace) {
    headers_t *headers = new_headers();
    cr_assert_not_null(headers, "Headers should not be null");

    const char *data = "Host:        localhost:42069        \r\n\r\n";
    parse_result_t result = parse_headers(headers, data, strlen(data));

    cr_assert_eq(result.error, NULL, "Expected no error");
    cr_assert_not_null(headers, "Headers should not be null");

    const char *host_value = headers_get(headers, "Host");
    cr_assert_str_eq(host_value, "localhost:42069");
    cr_assert_eq(result.n, 38, "Expected 38 bytes consumed");
    cr_assert_eq(result.done, false, "Expected done to be false");

    free_headers(headers);
}

Test(headers, valid_2_headers_with_existing_headers) {
    headers_t *headers = new_headers();
    cr_assert_not_null(headers, "Headers should not be null");

    // Add existing header (you'll need a helper function for this)
    int ret;
    khint_t k = kh_put(headers, headers->map, strdup("User-Agent"), &ret);
    kh_val(headers->map, k) = strdup("TestAgent");

    const char *data = "Host: localhost:42069\r\nContent-Type: application/json\r\n\r\n";
    parse_result_t result = parse_headers(headers, data, strlen(data));

    cr_assert_eq(result.error, NULL, "Expected no error");

    const char *host_value = headers_get(headers, "Host");
    cr_assert_str_eq(host_value, "localhost:42069");

    const char *user_agent_value = headers_get(headers, "User-Agent");
    cr_assert_str_eq(user_agent_value, "TestAgent");

    cr_assert_eq(result.n, 23, "Expected 23 bytes consumed");
    cr_assert_eq(result.done, false, "Expected done to be false");

    // Parse second header
    const char *remaining_data = data + result.n;
    size_t remaining_len = strlen(data) - result.n;
    parse_result_t result2 = parse_headers(headers, remaining_data, remaining_len);

    cr_assert_eq(result2.error, NULL, "Expected no error on second parse");

    const char *content_type_value = headers_get(headers, "Content-Type");
    cr_assert_str_eq(content_type_value, "application/json");

    cr_assert_eq(result2.n, 32, "Expected 32 bytes consumed");
    cr_assert_eq(result2.done, false, "Expected done to be false");

    free_headers(headers);
}

Test(headers, valid_done) {
    headers_t *headers = new_headers();
    cr_assert_not_null(headers, "Headers should not be null");

    const char *data = "\r\n";
    parse_result_t result = parse_headers(headers, data, strlen(data));

    cr_assert_eq(result.error, NULL, "Expected no error");
    cr_assert_eq(result.n, 2, "Expected 2 bytes consumed");
    cr_assert_eq(result.done, true, "Expected done to be true");
    cr_assert_eq(kh_size(headers->map), 0, "Expected headers map to be empty");

    free_headers(headers);
}

Test(headers, invalid_spacing_header) {
    headers_t *headers = new_headers();
    cr_assert_not_null(headers, "Headers should not be null");

    const char *data = "Host : localhost:42069\r\n\r\n";
    parse_result_t result = parse_headers(headers, data, strlen(data));

    cr_assert_not_null(result.error, "Expected error");
    cr_assert(strstr(result.error, "invalid header field name") != NULL,
              "Error should contain 'invalid header field name'");
    cr_assert_eq(result.n, 0, "Expected 0 bytes consumed");
    cr_assert_eq(result.done, false, "Expected done to be false");

    free(result.error);
    free_headers(headers);
}
