/*
 * collpr - replaces col|lpr for new nroff esp. when using tbl
 * Bill Joy UCB July 14, 1977
 *
 * This filter uses a screen buffer, 267 half-lines by 132 columns.
 * It interprets the up and down sequences generated by the new
 * nroff when used with tbl and by \u \d and \r.
 * General overstriking doesn't work correctly.
 * Option - causes output to go to std output rather than to lpr.
 */

char	page[267][132];

int	outline 1;
int	outcol;

extern	fout;
int	ibuf[259];

main(argc, argv)
	int argc;
	char *argv[];
{
	register c;
	register char *cp, *dp;
	int pvec[2], i;

	argc--;
	argv++;
	if (argc > 0 && argv[0][0] == '-') {
		argc--;
		argv++;
		fout = dup(1);
	} else {
		pipe(pvec);
		i = fork();
		if (i == -1) {
			write(2, "No more processes\n", 18);
			exit(1);
		}
		if (i == 0) {
			close(pvec[1]);
			close(0);
			dup(pvec[0]);
			close(pvec[0]);
			execl("/bin/lpr", "lpr", 0);
			execl("/usr/bin/lpr", "lpr", 0);
			perror("/usr/bin/lpr");
			exit(1);
		}
		close(pvec[0]);
		close(1);
		fout = pvec[1];
	}
	do {
		if (argc > 0) {
			close(0);
			if (fopen(argv[0], ibuf) < 0) {
				flush();
				perror(argv[0]);
				flush();
				exit(1);
			}
			argv++;
			argc--;
		}
		for (;;) {
			c = getc(ibuf);
			if (c == -1) {
				pflush(outline);
				flush();
				break;
			}
			switch (c) {
				case '\n':
					if (outline >= 265)
						pflush(62);
					outline =+ 2;
					outcol = 0;
					continue;
				case '\016':
				case '\017':
					continue;
				case 033:
					c = getc(ibuf);
					switch (c) {
						case '9':
							if (outline >= 266)
								pflush(62);
							outline++;
							continue;
						case '8':
							if (outline >= 1)
								outline--;
							continue;
						case '7':
							outline =- 2;
							if (outline < 0)
								outline = 0;
							continue;
						default:
							continue;
					}
				case '\b':
					if (outcol)
						outcol--;
					continue;
				case '\t':
					outcol =+ 8;
					outcol =& ~7;
					outcol--;
					c = ' ';
				default:
					if (outcol >= 132) {
						outcol++;
						continue;
					}
					cp = &page[outline][outcol];
again:
					if (c == ' ')
						;
					else if (*cp == 0) {
						*cp = c;
						dp = cp - outcol;
						for (cp--; cp >= dp && *cp == 0; cp--)
							*cp = ' ';
					} else if (*cp == ' ')
						*cp = c;
					else if (c == '_') {
under:
						if (outline & 1)
							cp =+ 132;
						else
							cp =- 132;
						if (*cp == 0 || *cp == ' ')
							goto again;
					} else if (*cp == '_') {
						*cp = c;
						c = '_';
						goto under;
					}
					outcol++;
					continue;
			}
		}
	} while (argc > 0);
}

pflush(ol)
	int ol;
{
	register int i, j;
	register char *cp;
	int l;

	l = ol;
	outline =- l;
	outcol = 0;
	if (l > 266)
		l = 266;
	else
		l =| 1;
	for (i = 1; i < l; i++) {
		cp = &page[i];
		switch (i & 1) {
			case 0:
				if (*cp)
					printf("\r%s", cp);
				putchar('\n');
				break;
			case 1:
				if (*cp)
					printf("%s", cp);
				break;
		}
	}
	copy(page, page[ol], (267 - ol) * 132);
	clear(page[267- ol], ol * 132);
}

plus(c, d)
{
	return (c == '|' && (d == '-' || d == '_'));
}

copy(to, from, i)
	register char *to, *from;
	register int i;
{

	if (i > 0)
		do
			*to++ = *from++;
		while (--i);
}

clear(at, cnt)
	register char *at;
	register int cnt;
{

	if (cnt > 0)
		do
			*at++ = 0;
		while (--cnt);
}
