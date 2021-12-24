/* Copyright (c) 2021, Daniel Florescu */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
	bool backslash_state = false, newline_flag = true;
	bool eof = false; /* End of current argument. */
	size_t len = 0;
	int c = 0, octal_state = 0;
	char octal_buffer[4] = {0}; /* "xxx\0" */
	unsigned int octal_number = 0;

	/* Read each argument. */
	for (int i = 1; i < argc; i++) {
		/* Parse charecter sequences. */
		len = strlen(argv[i]);
		for (size_t n = 0; n < len; n++) {
			if (n + 1 == len)
				eof = true;
			else
				eof = false;
			c = argv[i][n];

			if (octal_state > 0) {
				/* Support "\0[x[x[x]]]"
				 * When "\0" */
				if (octal_state != 4 && (c >= '0' && c <= '7') &&
				    !eof) {
					octal_buffer[octal_state - 1] = c;
					octal_state++;
				} else {
					if (eof) {
						octal_buffer[octal_state - 1] = c;
						octal_state++;
					}
					/* Parse octal into octal_number.
					 * We don't care if it fails. */
					sscanf(octal_buffer, "%o", &octal_number);
					/* Print octal as charecter. */
					putchar(octal_number);
					putchar(c);
					/* Reset everything. */
					octal_state = 0;
					octal_number = 0;
					memset(octal_buffer, 0, sizeof(octal_buffer));
				}
			} else if (backslash_state) {
				backslash_state = false;

				switch (c) {
				case '\\':
					putchar('\\');
					break;
				case 'a':
					putchar('\a');
					break;
				case 'b':
					putchar('\b');
					break;
				case 'c':
					newline_flag = false;
					break;
				case 'f':
					putchar('\f');
					break;
				case 'n':
					putchar('\n');
					break;
				case 'r':
					putchar('\r');
					break;
				case 't':
					putchar('\t');
					break;
				case 'v':
					putchar('\v');
					break;
				case '0':
					if (eof)
						putchar('\0');
					else
						octal_state = 1;
					break;
				default:
					putchar('\\');
					putchar(c);
				}
			} else if (c == '\\') {
				if (eof)
					putchar('\\');
				else
					backslash_state = true;
			} else {
				putchar(c);
			}
		}
		if (i + 1 != argc)
			putchar(' ');
	}
	if (newline_flag)
		putchar('\n');

	return 0;
}
