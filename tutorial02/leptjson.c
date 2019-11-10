#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <errno.h>
#include <math.h>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

#define IS_DIGIT(c) ((c)>='0'&&(c)<='9')

typedef struct {
	const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
	const char *p = c->json;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
		p++;
	c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, lept_type t, const char* literal)
{
	EXPECT(c, literal[0]);
	size_t id = 1;
	for (; literal[id]; ++id)
	{
		if (c->json[id - 1] != literal[id])
		{
			return LEPT_PARSE_INVALID_VALUE;
		}
	}
	c->json += id - 1;
	v->type = t;
	return LEPT_PARSE_OK;
}

static int consume_number(const char** s)
{
	const char* cursor = *s;
	while (IS_DIGIT(*cursor))
	{
		++cursor;
	}
	if (*s == cursor)
	{
		return 0;
	}
	*s = cursor;
	return 1;
}

static int is_valid_number(const char* s, char** end)	//end is returned only if number is valid 
{
	if (s[0] == '-')
	{
		++s;
	}
	if (s[0] == '0')
	{
		++s;
	}
	else if (!consume_number(&s))
	{
		return 0;
	}

	if (s[0] == '.' && !(++s, consume_number(&s)))
	{
		return 0;
	}

	if ((s[0] == 'E' || s[0]=='e'))
	{
		++s;
		if (s[0] == '+' || s[0] == '-')
		{
			++s;
		}
		if (!consume_number(&s))
		{
			return 0;
		}
	}
	*end = s;
	return 1;
		
}

static int lept_parse_number(lept_context* c, lept_value* v) {
	char* end;
	
	/* \TODO validate number */
	if (!is_valid_number(c->json, &end))
	{
		return LEPT_PARSE_INVALID_VALUE;
	}
	char tc = *end;	
	*end = '\0';		//trick here£¡just to make sure **strtod** doesn't convert a longer substring
	v->n = strtod(c->json, &end);
	*end = tc;			//restore the actual end character
	if (c->json == end)		
		return LEPT_PARSE_INVALID_VALUE;

	if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
	{
		errno = 0;
		return LEPT_PARSE_NUMBER_TOO_BIG;
	}
	c->json = end;
	v->type = LEPT_NUMBER;
	return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
	switch (*c->json) {
	case 't':  return lept_parse_literal(c, v, LEPT_TRUE, "true");
	case 'f':  return lept_parse_literal(c, v, LEPT_FALSE, "false");
	case 'n':  return lept_parse_literal(c, v, LEPT_NULL, "null");
	default:   return lept_parse_number(c, v);
	case '\0': return LEPT_PARSE_EXPECT_VALUE;
	}
}

int lept_parse(lept_value* v, const char* json) {
	lept_context c;
	int ret;
	assert(v != NULL);
	c.json = json;
	v->type = LEPT_NULL;
	lept_parse_whitespace(&c);
	if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
		lept_parse_whitespace(&c);
		if (*c.json != '\0') {
			v->type = LEPT_NULL;
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	return ret;
}

lept_type lept_get_type(const lept_value* v) {
	assert(v != NULL);
	return v->type;
}

double lept_get_number(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->n;
}
