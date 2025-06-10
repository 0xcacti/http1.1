#include "internal/headers/headers.h"
#include "internal/request/request.h"
#include <criterion/criterion.h>
#include <criterion/new/assert.h>
#include <string.h>

typedef struct {
    const char *data;
    size_t length;
    size_t num_bytes_per_read;
    size_t pos;
} chunk_reader_context_t;

ssize_t chunk_reader(void *context, char *buffer, size_t max_bytes) {
    chunk_reader_context_t *ctx = (chunk_reader_context_t *)context;
    if (ctx->pos >= ctx->length) {
        return 0;
    }

    size_t end_index = ctx->pos + ctx->num_bytes_per_read;
    if (end_index > ctx->length) {
        end_index = ctx->length;
    }

    size_t to_copy = end_index - ctx->pos;
    if (to_copy > max_bytes) {
        to_copy = max_bytes;
    }

    memcpy(buffer, ctx->data + ctx->pos, to_copy);
    ctx->pos += to_copy;
    return (ssize_t)to_copy;
}

Test(request, good_get_request_root) {
    char *request_data =
        "GET / HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n";

    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 3, .pos = 0};

    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);

    cr_assert_eq(result, 0, "Expected successful parsing");
    cr_assert_not_null(request.request_line, "Request line should not be null");
    cr_assert_str_eq(request.request_line->method, "GET");
    cr_assert_str_eq(request.request_line->request_target, "/");
    cr_assert_str_eq(request.request_line->http_version, "1.1");

    free_request(&request);
}

Test(request, good_get_request_with_path) {
    char *request_data = "GET /coffee HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: "
                         "curl/7.81.0\r\nAccept: */*\r\n\r\n";

    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 1, .pos = 0};

    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);

    cr_assert_eq(result, 0, "Expected successful parsing");
    cr_assert_not_null(request.request_line, "Request line should not be null");
    cr_assert_str_eq(request.request_line->method, "GET");
    cr_assert_str_eq(request.request_line->request_target, "/coffee");
    cr_assert_str_eq(request.request_line->http_version, "1.1");

    free_request(&request);
}

Test(request, good_post_request_with_path) {
    char *request_data = "POST /coffee HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: "
                         "curl/7.81.0\r\nAccept: */*\r\n\r\n";

    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 7, .pos = 0};

    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);

    cr_assert_eq(result, 0, "Expected successful parsing");
    cr_assert_not_null(request.request_line, "Request line should not be null");
    cr_assert_str_eq(request.request_line->method, "POST");
    cr_assert_str_eq(request.request_line->request_target, "/coffee");
    cr_assert_str_eq(request.request_line->http_version, "1.1");

    free_request(&request);
}

Test(request, invalid_number_of_parts_missing_method) {
    char *request_data = "/coffee HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: "
                         "curl/7.81.0\r\nAccept: */*\r\n\r\n";

    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 2, .pos = 0};

    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);

    cr_assert_eq(result, -1, "Expected parsing to fail");
}

Test(request, invalid_method_out_of_order) {
    char *request_data = "/coffee POST HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: "
                         "curl/7.81.0\r\nAccept: */*\r\n\r\n";

    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 1, .pos = 0};

    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);

    cr_assert_eq(result, -1, "Expected parsing to fail for invalid method");
}

Test(request, invalid_version_in_request_line) {
    char *request_data = "OPTIONS /prime/rib TCP/1.1\r\nHost: localhost:42069\r\nUser-Agent: "
                         "curl/7.81.0\r\nAccept: */*\r\n\r\n";

    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 1, .pos = 0};

    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);

    cr_assert_eq(result, -1, "Expected parsing to fail for invalid HTTP version");
}

Test(request, method_not_uppercase) {
    char *request_data = "get /test HTTP/1.1\r\nHost: localhost:42069\r\n\r\n";

    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 1, .pos = 0};

    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);

    cr_assert_eq(result, -1, "Expected parsing to fail for lowercase method");
}

Test(request, method_with_numbers) {
    char *request_data = "GET123 /test HTTP/1.1\r\nHost: localhost:42069\r\n\r\n";

    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 1, .pos = 0};

    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);

    cr_assert_eq(result, -1, "Expected parsing to fail for method with numbers");
}

Test(request, header_parse_standard_headers) {
    char *request_data =
        "GET / HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0\r\nAccept: */*\r\n\r\n";

    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 3, .pos = 0};

    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);

    cr_assert_eq(result, 0, "Expected successful parsing");
    cr_assert_not_null(request.headers, "Headers should not be null");

    const char *host = headers_get(request.headers, "host");
    const char *user_agent = headers_get(request.headers, "user-agent");
    const char *accept = headers_get(request.headers, "accept");

    cr_assert_str_eq(host, "localhost:42069");
    cr_assert_str_eq(user_agent, "curl/7.81.0");
    cr_assert_str_eq(accept, "*/*");

    free_request(&request);
}

Test(request, header_parse_malformed_header) {
    char *request_data = "GET / HTTP/1.1\r\nHost localhost:42069\r\n\r\n";

    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 3, .pos = 0};

    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);

    cr_assert_eq(result, -1, "Expected parsing to fail for malformed header");
}

Test(request, header_parse_empty_headers) {
    char *request_data = "GET / HTTP/1.1\r\n\r\n";

    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 3, .pos = 0};

    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);

    cr_assert_eq(result, 0, "Expected successful parsing");
    cr_assert_not_null(request.headers, "Headers should not be null");

    // Check that no headers are present by testing a common header
    const char *host = headers_get(request.headers, "host");
    cr_assert_null(host, "No headers should be present");

    free_request(&request);
}

Test(request, header_parse_duplicate_headers) {
    char *request_data = "GET / HTTP/1.1\r\nHost: localhost:42069\r\nHost: example.com\r\n\r\n";

    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 3, .pos = 0};

    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);

    cr_assert_eq(result, 0, "Expected successful parsing");
    cr_assert_not_null(request.headers, "Headers should not be null");

    const char *host = headers_get(request.headers, "host");
    cr_assert_str_eq(host, "localhost:42069, example.com");

    free_request(&request);
}

Test(request, header_parse_case_insensitive_headers) {
    char *request_data = "GET / HTTP/1.1\r\nHosT: localhost:42069\r\nhOSt: example.com\r\n\r\n";

    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 3, .pos = 0};

    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);

    cr_assert_eq(result, 0, "Expected successful parsing");
    cr_assert_not_null(request.headers, "Headers should not be null");

    const char *host = headers_get(request.headers, "host");
    cr_assert_str_eq(host, "localhost:42069, example.com");

    free_request(&request);
}

Test(request, header_parse_missing_end_of_headers) {
    char *request_data = "GET / HTTP/1.1\r\nHost: localhost:42069\r\nUser-Agent: curl/7.81.0";

    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 3, .pos = 0};

    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);

    cr_assert_eq(result, -1, "Expected parsing to fail for incomplete headers");
}

Test(request, parse_body_standard_body) {
    char *request_data = "POST /submit HTTP/1.1\r\nHost: localhost:42069\r\nContent-Length: "
                         "13\r\n\r\nhello world!\n";
    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 3, .pos = 0};
    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);
    cr_assert_eq(result, 0, "Expected successful parsing");
}

Test(request, parse_body_shorter_than_content_length) {
    char *request_data = "POST /submit HTTP/1.1\r\nHost: localhost:42069\r\nContent-Length: "
                         "20\r\n\r\npartial content";
    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 3, .pos = 0};
    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);
    cr_assert_eq(result, -1, "Expected parsing to fail for body shorter than Content-Length");
}

// func TestBodyParse_EmptyBodyZeroContentLength(t *testing.T){
//     reader : = &chunkReader{
//         data : "POST /submit HTTP/1.1\r\nHost: localhost:42069\r\nContent-Length: 0\r\n\r\n",
//         numBytesPerRead : 3,
//     } r,
//     err : = RequestFromReader(reader) require.NoError(t, err) require.NotNil(t, r)
//                 assert.Empty(t, r.Body)
// }

Test(request, parse_body_empty_body_zero_content_length) {
    char *request_data =
        "POST /submit HTTP/1.1\r\nHost: localhost:42069\r\nContent-Length: 0\r\n\r\n";
    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 3, .pos = 0};
    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);
    cr_assert_eq(result, 0, "Expected successful parsing");
    cr_assert_null(request.body);
    free_request(&request);
}

Test(request, parse_body_empty_body_no_reported_content_length) {
    char *request_data = "POST /submit HTTP/1.1\r\nHost: localhost:42069\r\n\r\n";
    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 3, .pos = 0};
    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);
    cr_assert_eq(result, 0, "Expected successful parsing");
    printf("Body: '%s'\n", request.body);
    cr_assert_null(request.body);
    free_request(&request);
}

Test(request, parse_body_no_content_length_with_body) {
    char *request_data = "POST /submit HTTP/1.1\r\nHost: localhost:42069\r\n\r\nhello world!";
    chunk_reader_context_t ctx = {
        .data = request_data, .length = strlen(request_data), .num_bytes_per_read = 3, .pos = 0};
    request_t request;
    int result = request_from_reader(chunk_reader, &ctx, &request);
    cr_assert_eq(result, 0, "Expected successful parsing");
    cr_assert_null(request.body);
    free_request(&request);
}
