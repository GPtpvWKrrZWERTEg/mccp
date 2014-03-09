#include <mccp/mccp.h>





int
main(int argc, const char *const argv[]) {
  mccp_result_t n_tokens;
  char *tokens[1024];
  ssize_t i;

  const char *a =
    "\\\\"
    "'that\\'s what I want.'"
    "\"that's what I want, too\""
    "\"who is the \\\"bad\\\"\""
    ;
  char *x;
  char *y;
  mccp_result_t len;

  (void)argc;
  (void)argv;

  x = strdup(a);
  n_tokens = mccp_str_tokenize_quote(x, tokens, 1024, "\t\r\n ", "\"'");
  if (n_tokens > 0) {

    for (i = 0; i < n_tokens; i++) {
      fprintf(stderr, "<%s>\n", tokens[i]);
    }

    fprintf(stderr, "\n");

    for (i = 0; i < n_tokens; i++) {
      if ((len = mccp_str_unescape(tokens[i], "\"'", &y)) >= 0) {
        fprintf(stderr, "[%s]\n", y);
        free((void *)y);
      } else {
        mccp_perror(len, "mccp_str_unescape()");
      }
    }

  } else {
    mccp_perror(n_tokens, "mccp_str_tokenize_quote()");
  }

  fprintf(stderr, "\n");

  x = strdup(a);
  n_tokens = mccp_str_tokenize(x, tokens, 1024, "\t\r\n ");
  if (n_tokens > 0) {
    for (i = 0; i < n_tokens; i++) {
      fprintf(stderr, "<%s>\n", tokens[i]);
    }
  } else {
    mccp_perror(n_tokens, "mccp_str_tokenize()");
  }

  if ((len = mccp_str_unescape("\"aaa nnn\"", "\"'", &y)) >= 0) {
    fprintf(stderr, "(%s)\n", y);
    free((void *)y);
  } else {
    mccp_perror(len, "mccp_str_unescape()");
  }

  return 0;
}
