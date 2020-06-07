#! /usr/bin/env python3

import os
import sys
import time
import optparse
import subprocess
import signal
import shutil
import glob
import numpy as np
import stats as st

STRING_DELIM = "\n=====================================\n"

DEFAULT_TARGET = "mean-results.csv"
DEFAULT_OUTPUT = ".csv"

def parse_cmd_args(argv):
    parser = optparse.OptionParser()
    parser.add_option("-d", "--directory", action="store", type="string", dest="dest_dir", default="",
                      metavar="DESTDIR",
                      help="Target directory where experiments results store")

    parser.add_option("-o", "--output", action="store", type="string", dest="output", default=DEFAULT_OUTPUT,
                      metavar="OUTPUT",
                      help="out file")

    parser.add_option("-f", "--file-name", action="store", type="string", dest="target", default=DEFAULT_TARGET,
                      metavar="TARGET",
                      help="target file with scalars")

    (options, args) = parser.parse_args()
    return options
    pass

###################################################################33

def create_NV_matrix(dest_dir, mobility, routing, target=DEFAULT_TARGET, output=DEFAULT_OUTPUT):
	init_dir = os.getcwd()
	print("Go to target directory %s..." % dest_dir)
	os.chdir(dest_dir)
	exp_mask = mobility + "-" + routing + "-*"
	print("Search experimentation directories by mask '%s'..." % exp_mask)
	exp_dirs = [d for d in glob.glob(exp_mask) if os.path.isdir(d)]
	if not exp_dirs:
	    print("Error! Target directory '%s' doesn't contain any exp directories!" % dest_dir)
	    os.chdir(init_dir)
	    return 1

	print("Found %d dirs! Process one-by-one..." % len(seed_dirs))


	return 0

if __name__ == '__main__':
    print('Script %s starts in directory %s%s' % (sys.argv[0], os.getcwd(), STRING_DELIM))

    options = parse_cmd_args(sys.argv)
    rc = run_calc(options.dest_dir, options.target, options.output)

    print('Done with rc = %d%s' % (rc, STRING_DELIM))
    sys.exit(rc)