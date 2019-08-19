/*
vnStat - Copyright (c) 2002-2019 Teemu Toivola <tst@iki.fi>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 dated June, 1991.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "common.h"
#include "ifinfo.h"
#include "dbsql.h"
#include "misc.h"
#include "cfg.h"
#include "cfgoutput.h"
#include "ibw.h"
#include "vnstat_func.h"

int main(int argc, char *argv[])
{
	int currentarg;
	DIR *dir = NULL;
	PARAMS p;

	initparams(&p);

	/* early check for debug and config parameter */
	if (argc > 1) {
		for (currentarg = 1; currentarg < argc; currentarg++) {
			if ((strcmp(argv[currentarg], "-D") == 0) || (strcmp(argv[currentarg], "--debug") == 0)) {
				debug = 1;
				printf("Debug enabled, vnstat %s\n", VERSION);
			} else if (strcmp(argv[currentarg], "--config") == 0) {
				if (currentarg + 1 < argc) {
					strncpy_nt(p.cfgfile, argv[currentarg + 1], 512);
					if (debug)
						printf("Used config file: %s\n", p.cfgfile);
					currentarg++;
					continue;
				} else {
					printf("Error: File for --config missing.\n");
					return 1;
				}
			}
		}
	}

	/* load config if available */
	if (!loadcfg(p.cfgfile)) {
		return 1;
	}
	if (!ibwloadcfg(p.cfgfile)) {
		return 1;
	}

	configlocale();
	strncpy_nt(p.interface, "default", 32);
	strncpy_nt(p.definterface, cfg.iface, 32);
	strncpy_nt(p.alias, "none", 32);

	/* parse parameters, maybe not the best way but... */
	for (currentarg = 1; currentarg < argc; currentarg++) {
		if (debug)
			printf("arg %d: \"%s\"\n", currentarg, argv[currentarg]);
		if (strcmp(argv[currentarg], "--longhelp") == 0) {
			showlonghelp(&p);
			return 0;
		} else if ((strcmp(argv[currentarg], "-?") == 0) || (strcmp(argv[currentarg], "--help") == 0)) {
			showhelp(&p);
			return 0;
		} else if ((strcmp(argv[currentarg], "-i") == 0) || (strcmp(argv[currentarg], "--iface") == 0)) {
			if (currentarg + 1 < argc) {
				if (strlen(argv[currentarg + 1]) > 31) {
					printf("Error: Interface name is limited to 31 characters.\n");
					return 1;
				}
				strncpy_nt(p.interface, argv[currentarg + 1], 32);
				if (strlen(p.interface)) {
					p.defaultiface = 0;
				} else {
					strncpy_nt(p.definterface, p.interface, 32);
				}
				if (debug)
					printf("Used interface: \"%s\"\n", p.interface);
				currentarg++;
				continue;
			} else {
				printf("Error: Interface for %s missing.\n", argv[currentarg]);
				return 1;
			}
		} else if (strcmp(argv[currentarg], "--config") == 0) {
			/* config has already been parsed earlier so nothing to do here */
			currentarg++;
			continue;
		} else if (strcmp(argv[currentarg], "--setalias") == 0 || strcmp(argv[currentarg], "--nick") == 0) {
			if (currentarg + 1 < argc) {
				strncpy_nt(p.alias, argv[currentarg + 1], 32);
				if (debug)
					printf("Used alias: \"%s\"\n", p.alias);
				p.setalias = 1;
				currentarg++;
				continue;
			} else {
				printf("Error: Alias for %s missing.\n", argv[currentarg]);
				return 1;
			}
		} else if ((strcmp(argv[currentarg], "--style")) == 0) {
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				if (cfg.ostyle > 4 || cfg.ostyle < 0) {
					printf("Error: Invalid style parameter \"%d\" for --style.\n", cfg.ostyle);
					printf(" Valid parameters:\n");
					printf("    0 - a more narrow output\n");
					printf("    1 - enable bar column if available\n");
					printf("    2 - average traffic rate in summary output\n");
					printf("    3 - average traffic rate in all outputs if available\n");
					printf("    4 - disable terminal control characters in -l / --live\n");
					printf("        and show raw values in --oneline\n");
					return 1;
				}
				cfg.ostyle = atoi(argv[currentarg + 1]);
				if (debug)
					printf("Style changed: %d\n", cfg.ostyle);
				currentarg++;
				continue;
			} else {
				printf("Error: Style parameter for --style missing.\n");
				printf(" Valid parameters:\n");
				printf("    0 - a more narrow output\n");
				printf("    1 - enable bar column if available\n");
				printf("    2 - average traffic rate in summary output\n");
				printf("    3 - average traffic rate in all outputs if available\n");
				printf("    4 - disable terminal control characters in -l / --live\n");
				printf("        and show raw values in --oneline\n");
				return 1;
			}
		} else if ((strcmp(argv[currentarg], "--dbdir")) == 0) {
			if (currentarg + 1 < argc) {
				strncpy_nt(cfg.dbdir, argv[currentarg + 1], 512);
				if (debug)
					printf("DatabaseDir: \"%s\"\n", cfg.dbdir);
				currentarg++;
				continue;
			} else {
				printf("Error: Directory for %s missing.\n", argv[currentarg]);
				return 1;
			}
		} else if ((strcmp(argv[currentarg], "--locale")) == 0) {
			if (currentarg + 1 < argc) {
				setlocale(LC_ALL, argv[currentarg + 1]);
				if (debug)
					printf("Locale: \"%s\"\n", argv[currentarg + 1]);
				currentarg++;
				continue;
			} else {
				printf("Error: Locale for %s missing.\n", argv[currentarg]);
				return 1;
			}
		} else if (strcmp(argv[currentarg], "--add") == 0) {
			p.addiface = 1;
			p.query = 0;
		} else if ((strcmp(argv[currentarg], "-u") == 0) || (strcmp(argv[currentarg], "--update") == 0)) {
			printf("Error: The \"%s\" parameter is not supported in this version.\n", argv[currentarg]);
			return 1;
		} else if ((strcmp(argv[currentarg], "-q") == 0) || (strcmp(argv[currentarg], "--query") == 0)) {
			p.query = 1;
		} else if ((strcmp(argv[currentarg], "-D") == 0) || (strcmp(argv[currentarg], "--debug") == 0)) {
			debug = 1;
		} else if ((strcmp(argv[currentarg], "-d") == 0) || (strcmp(argv[currentarg], "--days") == 0)) {
			cfg.qmode = 1;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listdays = atoi(argv[currentarg + 1]);
				if (cfg.listdays < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					return 1;
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-m") == 0) || (strcmp(argv[currentarg], "--months") == 0)) {
			cfg.qmode = 2;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listmonths = atoi(argv[currentarg + 1]);
				if (cfg.listmonths < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					return 1;
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-t") == 0) || (strcmp(argv[currentarg], "--top") == 0)) {
			cfg.qmode = 3;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listtop = atoi(argv[currentarg + 1]);
				if (cfg.listtop < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					return 1;
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-s") == 0) || (strcmp(argv[currentarg], "--short") == 0)) {
			cfg.qmode = 5;
		} else if ((strcmp(argv[currentarg], "-y") == 0) || (strcmp(argv[currentarg], "--years") == 0)) {
			cfg.qmode = 6;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listyears = atoi(argv[currentarg + 1]);
				if (cfg.listyears < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					return 1;
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-hg") == 0) || (strcmp(argv[currentarg], "--hoursgraph") == 0)) {
			cfg.qmode = 7;
		} else if ((strcmp(argv[currentarg], "-h") == 0) || (strcmp(argv[currentarg], "--hours") == 0)) {
			cfg.qmode = 11;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listhours = atoi(argv[currentarg + 1]);
				if (cfg.listhours < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					return 1;
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-5") == 0) || (strcmp(argv[currentarg], "--fiveminutes") == 0)) {
			cfg.qmode = 12;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listfivemins = atoi(argv[currentarg + 1]);
				if (cfg.listfivemins < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					return 1;
				}
				currentarg++;
			}
		} else if (strcmp(argv[currentarg], "--oneline") == 0) {
			cfg.qmode = 9;
			if (currentarg + 1 < argc && argv[currentarg + 1][0] != '-') {
				if (argv[currentarg + 1][0] == 'b') {
					cfg.ostyle = 4;
					currentarg++;
				} else {
					printf("Error: Invalid mode parameter \"%s\" for --oneline.\n", argv[currentarg + 1]);
					printf(" Valid parameters:\n");
					printf("    (none) - automatically scaled units visible\n");
					printf("    b      - all values in bytes\n");
					return 1;
				}
			}
		} else if (strcmp(argv[currentarg], "--xml") == 0) {
			if (currentarg + 1 < argc && argv[currentarg + 1][0] != '-' && !isdigit(argv[currentarg + 1][0])) {
				p.xmlmode = argv[currentarg + 1][0];
				if (strlen(argv[currentarg + 1]) != 1 || strchr("afhdmyt", p.xmlmode) == NULL) {
					printf("Error: Invalid mode parameter \"%s\" for --xml.\n", argv[currentarg + 1]);
					printf(" Valid parameters:\n");
					printf("    a - all (default)\n");
					printf("    f - only 5 minutes\n");
					printf("    h - only hours\n");
					printf("    d - only days\n");
					printf("    m - only months\n");
					printf("    y - only years\n");
					printf("    t - only top\n");
					return 1;
				}
				currentarg++;
			}
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listjsonxml = atoi(argv[currentarg + 1]);
				if (cfg.listjsonxml < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					return 1;
				}
				currentarg++;
			}
			cfg.qmode = 8;
		} else if (strcmp(argv[currentarg], "--json") == 0) {
			if (currentarg + 1 < argc && argv[currentarg + 1][0] != '-' && !isdigit(argv[currentarg + 1][0])) {
				p.jsonmode = argv[currentarg + 1][0];
				if (strlen(argv[currentarg + 1]) != 1 || strchr("afhdmyt", p.jsonmode) == NULL) {
					printf("Error: Invalid mode parameter \"%s\" for --json.\n", argv[currentarg + 1]);
					printf(" Valid parameters:\n");
					printf("    a - all (default)\n");
					printf("    f - only 5 minutes\n");
					printf("    h - only hours\n");
					printf("    d - only days\n");
					printf("    m - only months\n");
					printf("    y - only years\n");
					printf("    t - only top\n");
					return 1;
				}
				currentarg++;
			}
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listjsonxml = atoi(argv[currentarg + 1]);
				if (cfg.listjsonxml < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					return 1;
				}
				currentarg++;
			}
			cfg.qmode = 10;
		} else if ((strcmp(argv[currentarg], "-ru") == 0) || (strcmp(argv[currentarg], "--rateunit")) == 0) {
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				if (cfg.rateunit > 1 || cfg.rateunit < 0) {
					printf("Error: Invalid parameter \"%d\" for --rateunit.\n", cfg.rateunit);
					printf(" Valid parameters:\n");
					printf("    0 - bytes\n");
					printf("    1 - bits\n");
					return 1;
				}
				cfg.rateunit = atoi(argv[currentarg + 1]);
				if (debug)
					printf("Rateunit changed: %d\n", cfg.rateunit);
				currentarg++;
				continue;
			} else {
				cfg.rateunit = !cfg.rateunit;
				if (debug)
					printf("Rateunit changed: %d\n", cfg.rateunit);
			}
		} else if ((strcmp(argv[currentarg], "-tr") == 0) || (strcmp(argv[currentarg], "--traffic") == 0)) {
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.sampletime = atoi(argv[currentarg + 1]);
				currentarg++;
				p.traffic = 1;
				p.query = 0;
				continue;
			}
			p.traffic = 1;
			p.query = 0;
		} else if ((strcmp(argv[currentarg], "-l") == 0) || (strcmp(argv[currentarg], "--live") == 0)) {
			if (currentarg + 1 < argc && argv[currentarg + 1][0] != '-') {
				if (!isdigit(argv[currentarg + 1][0]) || p.livemode > 1 || p.livemode < 0) {
					printf("Error: Invalid mode parameter \"%s\" for -l / --live.\n", argv[currentarg + 1]);
					printf(" Valid parameters:\n");
					printf("    0 - show packets per second (default)\n");
					printf("    1 - show transfer counters\n");
					return 1;
				}
				p.livemode = atoi(argv[currentarg + 1]);
				currentarg++;
			}
			p.livetraffic = 1;
			p.query = 0;
		} else if (strcmp(argv[currentarg], "--force") == 0) {
			p.force = 1;
		} else if (strcmp(argv[currentarg], "--showconfig") == 0) {
			printcfgfile();
			return 0;
		} else if (strcmp(argv[currentarg], "--remove") == 0) {
			p.removeiface = 1;
			p.query = 0;
		} else if (strcmp(argv[currentarg], "--rename") == 0) {
			if (currentarg + 1 < argc) {
				strncpy_nt(p.newifname, argv[currentarg + 1], 32);
				if (debug)
					printf("Given new interface name: \"%s\"\n", p.newifname);
				p.renameiface = 1;
				p.query = 0;
				currentarg++;
				continue;
			} else {
				printf("Error: New interface name for %s missing.\n", argv[currentarg]);
				return 1;
			}
		} else if ((strcmp(argv[currentarg], "-b") == 0) || (strcmp(argv[currentarg], "--begin") == 0)) {
			if (currentarg + 1 < argc) {
				if (!validatedatetime(argv[currentarg + 1])) {
					printf("Error: Invalid date format, expected YYYY-MM-DD HH:MM or YYYY-MM-DD.\n");
					return 1;
				}
				strncpy_nt(p.databegin, argv[currentarg + 1], 18);
				currentarg++;
			} else {
				printf("Error: Date of format YYYY-MM-DD HH:MM or YYYY-MM-DD for %s missing.\n", argv[currentarg]);
				return 1;
			}
		} else if ((strcmp(argv[currentarg], "-e") == 0) || (strcmp(argv[currentarg], "--end") == 0)) {
			if (currentarg + 1 < argc) {
				if (!validatedatetime(argv[currentarg + 1])) {
					printf("Error: Invalid date format, expected YYYY-MM-DD HH:MM or YYYY-MM-DD.\n");
					return 1;
				}
				strncpy_nt(p.dataend, argv[currentarg + 1], 18);
				currentarg++;
			} else {
				printf("Error: Date of format YYYY-MM-DD HH:MM or YYYY-MM-DD for %s missing.\n", argv[currentarg]);
				return 1;
			}
		} else if (strcmp(argv[currentarg], "--iflist") == 0) {
			getifliststring(&p.ifacelist, 1);
			if (strlen(p.ifacelist)) {
				printf("Available interfaces: %s\n", p.ifacelist);
			} else {
				printf("No usable interfaces found.\n");
			}
			free(p.ifacelist);
			return 0;
		} else if ((strcmp(argv[currentarg], "-v") == 0) || (strcmp(argv[currentarg], "--version") == 0)) {
			printf("vnStat %s by Teemu Toivola <tst at iki dot fi>\n", getversion());
			return 0;
		} else {
			printf("Unknown parameter \"%s\". Use --help for help.\n", argv[currentarg]);
			return 1;
		}
	}

	/* open database and see if it contains any interfaces */
	if (!p.traffic && !p.livetraffic) {
		if ((dir = opendir(cfg.dbdir)) != NULL) {
			if (debug)
				printf("Dir OK\n");
			closedir(dir);
			if (!db_open_ro()) {
				printf("Error: Failed to open database \"%s/%s\" in read-only mode.\n", cfg.dbdir, DATABASEFILE);
				if (errno == ENOENT) {
					printf("The vnStat daemon should have created the database when started.\n");
					printf("Check that it is configured and running. See also \"man vnstatd\".\n");
				}
				return 1;
			}
			p.dbifcount = db_getinterfacecount();
			if (debug)
				printf("%" PRIu64 " interface(s) found\n", p.dbifcount);

			if (p.dbifcount > 1) {
				strncpy_nt(p.definterface, cfg.iface, 32);
			}
		} else {
			printf("Error: Unable to open database directory \"%s\": %s\n", cfg.dbdir, strerror(errno));
			if (errno == ENOENT) {
				printf("The vnStat daemon should have created this directory when started.\n");
				printf("Check that it is configured and running. See also \"man vnstatd\".\n");
			} else {
				printf("Make sure it is at least read enabled for current user.\n");
				printf("Use --help for help.\n");
			}
			return 1;
		}
	}

	/* set used interface if none specified */
	handleifselection(&p);

	/* parameter handlers */
	handleremoveinterface(&p);
	handlerenameinterface(&p);
	handleaddinterface(&p);
	handlesetalias(&p);
	handleshowdatabases(&p);
	handletrafficmeters(&p);

	/* show something if nothing was shown previously */
	if (!p.query && !p.traffic && !p.livetraffic) {

		/* give more help if there's no database */
		if (p.dbifcount == 0) {
			getifliststring(&p.ifacelist, 1);
			printf("No interfaces found in the database, nothing to do. Use --help for help.\n\n");
			printf("Interfaces can be added to the database with the following command:\n");
			printf("    %s --add -i eth0\n\n", argv[0]);
			printf("Replace 'eth0' with the interface that should be monitored.\n\n");
			if (strlen(cfg.cfgfile)) {
				printf("The default interface can be changed by updating the \"Interface\" keyword\n");
				printf("value in the configuration file \"%s\".\n\n", cfg.cfgfile);
			}
			printf("The following interfaces are currently available:\n    %s\n", p.ifacelist);
			free(p.ifacelist);
		} else {
			printf("Nothing to do. Use --help for help.\n");
		}
	}

	/* cleanup */
	db_close();
	ibwflush();

	return 0;
}
