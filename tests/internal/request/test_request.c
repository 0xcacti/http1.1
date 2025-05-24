#include "internal/request/request.h"
#include <criterion/criterion.h>
#include <criterion/new/assert.h>

Test(request, request_line_parse) {
    cr_assert_str_eq("TheTestagen", "TheTestagen");
}
