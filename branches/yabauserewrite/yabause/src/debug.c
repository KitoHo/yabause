#include "debug.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

Debug * DebugNew(const char * n, DebugOutType t, char * s) {
	Debug * d;

	d = (Debug *) malloc(sizeof(Debug));
	d->output_type = t;
	d->name = strdup(n);

	switch(t) {
	case DEBUG_STREAM:
		d->output.stream = fopen(s, "w");
		break;
	case DEBUG_STRING:
		d->output.string = s;
		break;
	case DEBUG_STDOUT:
		d->output.stream = stdout;
		break;
	case DEBUG_STDERR:
		d->output.stream = stderr;
		break;
	}

	return d;
}

void DebugDelete(Debug * d) {
	switch(d->output_type) {
	case DEBUG_STREAM:
		fclose(d->output.stream);
		break;
	case DEBUG_STRING:
	case DEBUG_STDOUT:
	case DEBUG_STDERR:
		break;
	}
	free(d->name);
	free(d);
}

void DebugChangeOutput(Debug * d, DebugOutType t, char * s) {
	if (t != d->output_type) {
		if (d->output_type == DEBUG_STREAM)
			fclose(d->output.stream);
		d->output_type = t;
	}
	switch(t) {
	case DEBUG_STREAM:
		d->output.stream = fopen(s, "w");
		break;
	case DEBUG_STRING:
		d->output.string = s;
		break;
	case DEBUG_STDOUT:
		d->output.stream = stdout;
		break;
	case DEBUG_STDERR:
		d->output.stream = stderr;
		break;
	}
}

void DebugPrintf(Debug * d, const char * file, u32 line, const char * format, ...) {
	va_list l;

	va_start(l, format);

	switch(d->output_type) {
	case DEBUG_STDOUT:
	case DEBUG_STDERR:
	case DEBUG_STREAM:
		fprintf(d->output.stream, "%s (%s:%ld): ", d->name, file, line);
		vfprintf(d->output.stream, format, l);
		break;
	case DEBUG_STRING:
		{
			int i;
			i = sprintf(d->output.string, "%s (%s:%ld): ", d->name, file, line);
			vsprintf(d->output.string + i, format, l);
		}
		break;
	}

	va_end(l);
}
