#include "internal/request/request.h"
#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <string.h>

Test(request, good_get_request_root) {
    char *request_data =
        "GET / HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n";
    request_t request;

    int result = request_from_header(request_data, strlen(request_data), &request);

    cr_assert_eq(result, 0, "Expected successful parsing");
    cr_assert_not_null(request.line, "Request line should not be null");
    cr_assert_str_eq(request.line->method, "GET");
    cr_assert_str_eq(request.line->request_target, "/");
    cr_assert_str_eq(request.line->http_version, "1.1");

    free_request(&request);
}

Test(request, good_get_request_with_path) {
    char *request_data = "GET /coffee HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: "
                         "curl/7.81.0\r\nAccept: */*\r\n\r\n";
    request_t request;

    int result = request_from_header(request_data, strlen(request_data), &request);

    cr_assert_eq(result, 0, "Expected successful parsing");
    cr_assert_not_null(request.line, "Request line should not be null");
    cr_assert_str_eq(request.line->method, "GET");
    cr_assert_str_eq(request.line->request_target, "/coffee");
    cr_assert_str_eq(request.line->http_version, "1.1");

    free_request(&request);
}

Test(request, good_post_request_with_path) {
    char *request_data = "POST /coffee HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: "
                         "curl/7.81.0\r\nAccept: */*\r\n\r\n";
    request_t request;

    int result = request_from_header(request_data, strlen(request_data), &request);

    cr_assert_eq(result, 0, "Expected successful parsing");
    cr_assert_not_null(request.line, "Request line should not be null");
    cr_assert_str_eq(request.line->method, "POST");
    cr_assert_str_eq(request.line->request_target, "/coffee");
    cr_assert_str_eq(request.line->http_version, "1.1");

    free_request(&request);
}

Test(request, invalid_number_of_parts_missing_method) {
    char *request_data = "/coffee HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: "
                         "curl/7.81.0\r\nAccept: */*\r\n\r\n";
    request_t request;

    int result = request_from_header(request_data, strlen(request_data), &request);

    cr_assert_eq(result, -1, "Expected parsing to fail");
}

Test(request, invalid_method_out_of_order) {
    char *request_data = "/coffee POST HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: "
                         "curl/7.81.0\r\nAccept: */*\r\n\r\n";
    request_t request;

    int result = request_from_header(request_data, strlen(request_data), &request);

    cr_assert_eq(result, -1, "Expected parsing to fail for invalid method");
}

Test(request, invalid_version_in_request_line) {
    char *request_data = "OPTIONS /prime/rib TCP/1.1\r\nHost: localhost:42069\r\nUser-Agent: "
                         "curl/7.81.0\r\nAccept: */*\r\n\r\n";
    request_t request;

    int result = request_from_header(request_data, strlen(request_data), &request);

    cr_assert_eq(result, -1, "Expected parsing to fail for invalid HTTP version");
}

Test(request, method_not_uppercase) {
    char *request_data = "get /test HTTP/1.1\r\nHost: localhost:42069\r\n\r\n";
    request_t request;

    int result = request_from_header(request_data, strlen(request_data), &request);

    cr_assert_eq(result, -1, "Expected parsing to fail for lowercase method");
}

Test(request, method_with_numbers) {
    char *request_data = "GET123 /test HTTP/1.1\r\nHost: localhost:42069\r\n\r\n";
    request_t request;

    int result = request_from_header(request_data, strlen(request_data), &request);

    cr_assert_eq(result, -1, "Expected parsing to fail for method with numbers");
}
