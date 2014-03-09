#ifndef __MCCP_STRUTILS_H__
#define __MCCP_STRUTILS_H__





/**
 *	@file	mccp_strutils.h
 */





mccp_result_t
mccp_str_tokenize(char *buf, char **tokens,
                  size_t max, const char *delm);

mccp_result_t
mccp_str_tokenize_quote(char *buf, char **tokens,
                        size_t max, const char *delm, const char *quote);

mccp_result_t
mccp_str_unescape(const char *org, const char *escaped,
                  char **retptr);

mccp_result_t
mccp_str_trim_right(const char *org, const char *trim_chars,
                    char **retptr);





mccp_result_t
mccp_str_parse_int16_by_base(const char *buf, int16_t *val,
                             unsigned int base);
mccp_result_t
mccp_str_parse_int16(const char *buf, int16_t *val);
mccp_result_t
mccp_str_parse_uint16_by_base(const char *buf, uint16_t *val,
                              unsigned int base);
mccp_result_t
mccp_str_parse_uint16(const char *buf, uint16_t *val);





mccp_result_t
mccp_str_parse_int32_by_base(const char *buf, int32_t *val,
                             unsigned int base);
mccp_result_t
mccp_str_parse_int32(const char *buf, int32_t *val);
mccp_result_t
mccp_str_parse_uint32_by_base(const char *buf, uint32_t *val,
                              unsigned int base);
mccp_result_t
mccp_str_parse_uint32(const char *buf, uint32_t *val);





mccp_result_t
mccp_str_parse_int64_by_base(const char *buf, int64_t *val,
                             unsigned int base);
mccp_result_t
mccp_str_parse_int64(const char *buf, int64_t *val);
mccp_result_t
mccp_str_parse_uint64_by_base(const char *buf, uint64_t *val,
                              unsigned int base);
mccp_result_t
mccp_str_parse_uint64(const char *buf, uint64_t *val);





mccp_result_t
mccp_str_parse_float(const char *buf, float *val);


mccp_result_t
mccp_str_parse_double(const char *buf, double *val);


mccp_result_t
mccp_str_parse_long_double(const char *buf, long double *val);


mccp_result_t
mccp_str_parse_bool(const char *buf, bool *val);





#endif /* ! __MCCP_STRUTILS_H__ */
