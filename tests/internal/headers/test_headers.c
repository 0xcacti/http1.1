#include "internal/headers/headers.h"
#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <string.h>

Test(headers, valid_single_header) {
    headers_t *headers = new_headers();
    cr_assert_not_null(headers, "Headers should not be null");

    const char *data = "Host: localhost:42069\r\n\r\n";
    parse_result_t result = parse_headers(headers, data, strlen(data));

    cr_assert_eq(result.error, NULL, "Expected no error");
    cr_assert_not_null(headers, "Headers should not be null");

    const char *host_value = headers_get(headers, "host");
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

    const char *host_value = headers_get(headers, "host");
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
    khint_t k = kh_put(headers, headers->map, strdup("user-agent"), &ret);
    kh_val(headers->map, k) = strdup("TestAgent");

    const char *data = "Host: localhost:42069\r\nContent-Type: application/json\r\n\r\n";
    parse_result_t result = parse_headers(headers, data, strlen(data));

    cr_assert_eq(result.error, NULL, "Expected no error");

    const char *host_value = headers_get(headers, "host");
    cr_assert_str_eq(host_value, "localhost:42069");

    const char *user_agent_value = headers_get(headers, "user-agent");
    cr_assert_str_eq(user_agent_value, "TestAgent");

    cr_assert_eq(result.n, 23, "Expected 23 bytes consumed");
    cr_assert_eq(result.done, false, "Expected done to be false");

    // Parse second header
    const char *remaining_data = data + result.n;
    size_t remaining_len = strlen(data) - result.n;
    parse_result_t result2 = parse_headers(headers, remaining_data, remaining_len);

    cr_assert_eq(result2.error, NULL, "Expected no error on second parse");

    const char *content_type_value = headers_get(headers, "content-type");
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

Test(headers, captal_header) {
    headers_t *headers = new_headers();
    cr_assert_not_null(headers, "Headers should not be null");

    const char *data = "Host: localhost:42069\r\n\r\n";
    parse_result_t result = parse_headers(headers, data, strlen(data));

    cr_assert_eq(result.error, NULL, "Expected no error");
    cr_assert_not_null(headers, "Headers should not be null");

    const char *host_value = headers_get(headers, "host");
    cr_assert_str_eq(host_value, "localhost:42069");
    cr_assert_eq(result.n, 23, "Expected 23 bytes consumed");
    cr_assert_eq(result.done, false, "Expected done to be false");

    free_headers(headers);
}

Test(headers, invalid_header_name_character) {
    headers_t *headers = new_headers();
    cr_assert_not_null(headers, "Headers should not be null");

    const char *data = "H©st: localhost:42069\r\n\r\n";
    parse_result_t result = parse_headers(headers, data, strlen(data));

    cr_assert_not_null(result.error, "Expected error");
    cr_assert(strstr(result.error, "invalid header field name") != NULL,
              "Error should contain 'invalid header field name'");
    cr_assert_eq(result.n, 0, "Expected 0 bytes consumed");
    cr_assert_eq(result.done, false, "Expected done to be false");

    free(result.error);
    free_headers(headers);
}

Test(headers, multiple_values_for_header) {
    headers_t *headers = new_headers();
    cr_assert_not_null(headers, "Headers should not be null");
    const char *data = "Host: localhost:42069\r\nHost: example.com\r\n\r\n";

    // First parse
    parse_result_t result1 = parse_headers(headers, data, strlen(data));
    cr_assert_null(result1.error, "Expected no error on first parse");
    cr_assert_str_eq(headers_get(headers, "host"), "localhost:42069", "Expected initial value");
    cr_assert_eq(result1.done, false, "Expected not done after first parse");
    cr_assert_eq(result1.n, 23, "Expected 23 bytes consumed");

    // Second parse (should concatenate)
    parse_result_t result2 = parse_headers(headers, data + result1.n, strlen(data) - result1.n);
    cr_assert_null(result2.error, "Expected no error on second parse");
    cr_assert_str_eq(headers_get(headers, "host"), "localhost:42069, example.com",
                     "Expected concatenated value");
    cr_assert_eq(result2.done, false, "Expected not done after second parse");

    // Third parse (should hit \r\n and set done = true)
    parse_result_t result3 =
        parse_headers(headers, data + result1.n + result2.n, strlen(data) - result1.n - result2.n);
    cr_assert_null(result3.error, "Expected no error on final parse");
    cr_assert_str_eq(headers_get(headers, "host"), "localhost:42069, example.com",
                     "Value should remain unchanged");
    cr_assert_eq(result3.done, true, "Expected done to be true");
    cr_assert_eq(result1.n + result2.n + result3.n, 44, "Expected total of 44 bytes consumed");

    free_headers(headers);
}
