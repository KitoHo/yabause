#include "core.h"

int StateWriteHeader(FILE *fp, const char *name, int version) {
	fprintf(fp, name);
	fwrite((void *)&version, sizeof(version), 1, fp);
	fwrite((void *)&version, sizeof(version), 1, fp); // place holder for size
	return ftell(fp);
}

int StateFinishHeader(FILE *fp, int offset) {
	int size = 0;
	size = ftell(fp) - offset;
	fseek(fp, offset - 4, SEEK_SET);
	fwrite((void *)&size, sizeof(size), 1, fp); // write true size
	fseek(fp, 0, SEEK_END);
	return (size + 12);
}

int StateCheckRetrieveHeader(FILE *fp, const char *name, int *version, int *size) {
	char id[4];
	int ret;

	if ((ret = fread((void *)id, 1, 4, fp)) != 4)
		return -1;

	if (strncmp(name, id, 4) != 0)
		return -2;

	if ((ret = fread((void *)version, 4, 1, fp)) != 1)
		return -1;

	if (fread((void *)size, 4, 1, fp) != 1)
		return -1;

	return 0;
}
