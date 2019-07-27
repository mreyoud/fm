/* see LICENSE file for copyright and license details. */

/* associations: regex(3) extensions and associated opener map */
static struct assoc assocs[] = {
	{ "\\.(avi|mp3|mp4|mkv|ogg|flac|webm)$", "mpv"     },
	{ "\\.(png|jpg|gif)$",                   "sxiv"    },
	{ "\\.(html)$",                          "firefox" },
	{ "\\.(pdf)$",                           "zathura" },
	{ "\\.(sh)$",                            "sh"      },
	{ ".",                                   "less"    },
};

/* bindings: (use "^X" for CTRL+X, and "M-X" for ALT+X */
static struct key keys[] = {
	/* key  proc   arg                desc                               */
	{ "d",  del,   { .b = false } },  /* delete selection                */
	{ "D",  del,   { .b = true } },   /* delete marked                   */
	{ "h",  nav,   { .i = -1 } },     /* exit directory                  */
	{ "j",  step,  { .i = +1 } },     /* move cursor up                  */
	{ "J",  step,  { .i = +10 } },    /* move cursor up by VALUE         */
	{ "k",  step,  { .i = -1 } },     /* move cursor down                */
	{ "K",  step,  { .i = -10 } },    /* move cursor down by VALUE       */
	{ "l",  nav,   { .i = +1 } },     /* enter directory                 */
	{ "n",  touch, { .b = false } },  /* new file                        */
	{ "N",  touch, { .b = true } },   /* new directory                   */
	{ "o",  with,  { 0 } },           /* open file (see accoc map)       */
	{ "q",  quit,  { 0 } },           /* bye                             */
	{ "r",  nav,   { .i = 0 } },      /* reload directory                */
	{ ".",  dot,   { 0 } },           /* toggle hidden                   */
	{ " ",  mark,  { 0 } },           /* toggle mark                     */
};

