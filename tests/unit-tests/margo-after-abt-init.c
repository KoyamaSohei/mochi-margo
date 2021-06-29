
#include <margo.h>
#include "helper-server.h"
#include "munit/munit.h"

struct test_context {
    margo_instance_id mid;
    char*             log_buffer;
    int               log_buffer_pos;
    int               log_buffer_size;
};

static void test_log_fn(void* uargs, const char* str)
{
    struct test_context* ctx = uargs;

    /* check for overflow */
    munit_assert_int(strlen(str) + ctx->log_buffer_pos, <,
                     ctx->log_buffer_size);

    /* directly copy msg to buffer */
    strcpy(&ctx->log_buffer[ctx->log_buffer_pos], str);
    ctx->log_buffer_pos += strlen(str);

    return;
}

struct margo_logger test_logger;

static void* test_context_setup(const MunitParameter params[], void* user_data)
{
    (void)params;
    (void)user_data;

    struct test_context* ctx = calloc(1, sizeof(*ctx));
    ctx->log_buffer          = calloc(102400, 1);
    ctx->log_buffer_size     = 102400;

    /* set up custom logger to make it easier to validate output */
    test_logger.uargs    = ctx;
    test_logger.trace    = test_log_fn;
    test_logger.debug    = test_log_fn;
    test_logger.info     = test_log_fn;
    test_logger.warning  = test_log_fn;
    test_logger.error    = test_log_fn;
    test_logger.critical = test_log_fn;

#if 0
    ret = margo_set_global_logger(&test_logger);
    munit_assert_int(ret, ==, 0);
#endif

    return ctx;
}

static void test_context_tear_down(void* data)
{
    struct test_context* ctx = (struct test_context*)data;

    free(ctx->log_buffer);
    free(ctx);
}

static MunitResult margo_after_abt(const MunitParameter params[], void* data)
{
    struct test_context* ctx = (struct test_context*)data;
    char* protocol = "na+sm";
    int ret;

    ret = ABT_init(0, NULL);
    munit_assert_int(ret, ==, 0);

    ctx->mid = margo_init(protocol, MARGO_CLIENT_MODE, 0, 0);
    munit_assert_not_null(ctx->mid);

    margo_finalize(ctx->mid);

    return MUNIT_OK;
}

static MunitTest tests[]
    = {{"/margo-after-abt", margo_after_abt, test_context_setup,
        test_context_tear_down, MUNIT_TEST_OPTION_NONE, NULL},
       {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

static const MunitSuite test_suite
    = {"/margo", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE};

int main(int argc, char** argv)
{
    return munit_suite_main(&test_suite, NULL, argc, argv);
}
