/* see LICENSE file for copyright and license details. */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <limits.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <ncurses.h>
#include <lib0.h>

enum { NONE = 0, QUIT = 1, LOAD = 2, DRAW = 4, DOTS = 8 };

union arg {
	int i;
	bool b;
	const void *v;
};

struct assoc {
	char *re;
	char *bin;
};

struct entry {
	char name[NAME_MAX];
	char perms[11];
	char size[10];
	char owner[10];
	char group[10];
	bool isdir;
	bool ismarked;
};

struct key {
	const char *key;
	void (*fn)(const union arg *);
	const union arg arg;
};

static int cmp(const void *, const void *);
static void curse(void);
static void del(const union arg *);
static void dot(const union arg *);
static void draw(void);
static void input(void);
static void load(void);
static void mark(const union arg *);
static void nav(const union arg *);
static void quit(const union arg *);
static void run(void);
static void setup(void);
static void spawn(const char *[]);
static void step(const union arg *);
static void touch(const union arg *);
static void with(const union arg *);

static char cwd[PATH_MAX], msg[BUFSIZ];
static int cur, nents;
static unsigned int flags = LOAD;
static struct entry *ents;

#include "config.h"

int cmp(const void *a, const void *b) {
	int r;
	if ((r = ((struct entry *)b)->isdir - ((struct entry *)a)->isdir))
		return r;
	return strcoll(((struct entry *)a)->name, ((struct entry *)b)->name);
}

void curse(void) {
	endwin();
	cbreak();
	noecho();
	keypad(stdscr, 1);
	curs_set(0);
	refresh();
	flags |= DRAW;
}

void del(const union arg *arg) {
#define DELETE(NAME) \
	spawn((const char *[]){"rm", "-rf", (NAME), NULL});
	if (!nents)
		return;
	strcpy(msg, "delete? (Y/n)");
	draw();
	flags |= DRAW;
	if (getch() != 'Y')
		return;
	flags |= LOAD;
	if (!arg->b) {
		DELETE(ents[cur].name);
		return;
	}
	for (int i = 0; i < nents; i++)
		if (ents[i].ismarked)
			DELETE(ents[i].name);
}

void dot(const union arg *arg) {
	USED(arg), flags = (flags ^ DOTS) | LOAD;
}

void draw(void) {
	int i, beg, end, nl, max;
	struct entry *p;
	erase();
	attr_on(A_BOLD, NULL);
	printw("%s\n", cwd);
	attr_off(A_BOLD, NULL);
	nl = MIN(LINES - 4, nents);
	if (cur < nl / 2) {
		beg = 0;
		end = nl;
	} else if (cur >= nents - nl / 2) {
		beg = nents - nl;
		end = nents;
	} else {
		beg = cur - nl / 2;
		end = cur + nl / 2 + (nl & 1);
	}
	for (max = 0, i = 0; i < nents; i++)
		max = MAX(max, strlen(ents[i].name));
	max = CAP(max, 0, COLS);
	for (i = beg; i < end; i++) {
		if (i == cur)
			attr_on(A_REVERSE, NULL);
		if ((p = &ents[i])->isdir)
			attr_on(A_BOLD, NULL);
		printw("%c %s%*s  \n", p->ismarked ? '*' : ' ',
			p->name, max - strlen(p->name), "");
		attr_off(A_BOLD | A_REVERSE, NULL);
	}
	if (msg[0] != '\0') {
		printw("%s\n", msg);
	}
	else if (nents) {
		p = &ents[cur];
		printw("%s:%s %s %s\n", p->owner, p->group, p->size, p->perms);
	}
	refresh();
	flags &= ~DRAW;
}

void input(void) {
	int c, i;
	if ((c = getch()) == KEY_RESIZE)
		curse();
	for (i = 0; i < (int)LEN(keys); i++)
		if (!strcmp(keys[i].key, keyname(c)))
			keys[i].fn(&(keys[i].arg));
}

void load(void) {
	static char units[] = {'B','K','M','G','T'};
	struct passwd *pw;
	struct group *gr;
	float sz;
	regex_t re;
	DIR *d;
	struct dirent *dp;
	struct stat sb;
	int i;
	free(ents);
	cur = 0, nents = 0, ents = NULL;
	getcwd(cwd, PATH_MAX);
	regcomp(&re, flags & DOTS ? "." : "^[^.]",
		REG_NOSUB|REG_EXTENDED|REG_ICASE);
	d = opendir(cwd);
	while ((dp = readdir(d))) {
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		if (regexec(&re, dp->d_name, 0, NULL, 0))
			continue;
		stat(dp->d_name, &sb);
		ents = realloc(ents, (nents + 1) * sizeof(struct entry));
		strcpy(ents[nents].name, dp->d_name);
		ents[nents].isdir = S_ISDIR(sb.st_mode);
		ents[nents].ismarked = 0;
		pw = getpwuid(sb.st_uid);
		strcpy(ents[nents].owner, pw->pw_name);
		gr = getgrgid(sb.st_gid);
		strcpy(ents[nents].group, gr->gr_name);
		for (sz = (float)sb.st_size, i = 0; sz > 1024 && i < 4; i++)
			sz /= 1024;
		sprintf(ents[nents].size, "%.3g%c", sz, units[i]);
		ents[nents].perms[0] = S_ISDIR(sb.st_mode)    ? 'd' : '-';
		ents[nents].perms[1] = (sb.st_mode & S_IRUSR) ? 'r' : '-';
		ents[nents].perms[2] = (sb.st_mode & S_IWUSR) ? 'w' : '-';
		ents[nents].perms[3] = (sb.st_mode & S_IXUSR) ? 'x' : '-';
		ents[nents].perms[4] = (sb.st_mode & S_IRGRP) ? 'r' : '-';
		ents[nents].perms[5] = (sb.st_mode & S_IWGRP) ? 'w' : '-';
		ents[nents].perms[6] = (sb.st_mode & S_IXGRP) ? 'x' : '-';
		ents[nents].perms[7] = (sb.st_mode & S_IROTH) ? 'r' : '-';
		ents[nents].perms[8] = (sb.st_mode & S_IWOTH) ? 'w' : '-';
		ents[nents].perms[9] = (sb.st_mode & S_IXOTH) ? 'x' : '-';
		ents[nents].perms[10] = '\0';
		nents++;
	}
	closedir(d);
	qsort(ents, nents, sizeof(struct entry), cmp);
	flags = (flags & ~LOAD) | DRAW;
}

void quit(const union arg *arg) {
	USED(arg), flags |= QUIT;
}

void mark(const union arg *arg) {
	if (!nents)
		return;
	ents[cur].ismarked = !ents[cur].ismarked;
	flags |= DRAW;
	USED(arg);
}

void nav(const union arg *arg) {
	chdir(arg->i == 0 ? "." : arg->i < 0 ? ".." : ents[cur].name);
	flags |= LOAD;
}

void run(void) {
	while (!(flags & QUIT)) {
		msg[0] = '\0';
		if (flags & LOAD)
			load();
		if (flags & DRAW)
			draw();
		input();
	}
}

void setup(void) {
	setlocale(LC_ALL, "");
	atexit((void (*)(void))endwin);
	initscr();
	curse();
}

void spawn(const char *arg[]) {
	pid_t pid;
	int r;
	pid = fork();
	if (pid > 0) {
		endwin();
		waitpid(pid, &r, 0);
		curse();
	} else if (pid == 0) {
		execvp((const char *)arg[0], (char * const *)arg);
		die("execvp %s failed\n", arg[0]);
	}
}

void step(const union arg *arg) {
	cur = CAP(cur + arg->i, 0, nents - 1), flags |= DRAW;
}

void touch(const union arg *arg) {
	char buf[100] = {0}, *s = buf;
	int c;
	do {
		sprintf(msg, "NAME: %s", buf);
		draw();
		c = getch();
		if (c == '\n') {
			goto out;
		}
		else if (c == KEY_BACKSPACE) {
			if (s > buf)
				*--s = 0;
		}
		else if (isprint(c)) {
			if (s - buf < (ptrdiff_t)sizeof buf - 1)
				*s++ = c, *s = 0;
		}
		else {
			goto out;
		}
	} while (1);
out:
	flags |= DRAW;
	if (buf[0] == '\0')
		return;
	spawn((const char*[]) {arg->b ? "mkdir" : "touch", buf, NULL});
	flags |= LOAD;
}

void with(const union arg *arg) {
	char *bin, cmd[PATH_MAX];
	regex_t re;
	size_t i;
	for (bin = NULL, i = 0; i < LEN(assocs); i++) {
		if (regcomp(&re, assocs[i].re,
			REG_NOSUB|REG_EXTENDED|REG_ICASE))
			continue;
		if (!regexec(&re, ents[cur].name, 0, NULL, 0)) {
			bin = assocs[i].bin;
			break;
		}
	}
	snprintf(cmd, PATH_MAX, "%s \"%s\"", bin, ents[cur].name);
	spawn((const char*[]){"sh", "-c", cmd, NULL});
	USED(arg);
}

int main(void) {
	setup();
	run();
	return 0;
}

